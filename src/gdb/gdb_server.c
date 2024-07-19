#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "core/riscv.h"
#include "plat/plat.h"          // 包含对应环境中的 BSD Socket 初始化

// 定义 command 全局缓存
static char debug_info_buffer[DEBUG_INFO_BUFFER_SIZE];      // 全部初始化为 0

// 定义 command-checksum
static uint8_t checksum = 0;

static char _char_to_hex(char c) {
    if ((c >= '0') && (c <= '9'))
        return c - '0';
    if ((c >= 'a') && (c <= 'f'))
        return c - 'a' + 10;
    if ((c >= 'A') && (c <= 'F'))
        return c - 'A' + 10;
    return 0;
}

// 辅助函数 把 checksum-str 转换为 checksum-num
static uint8_t _str_to_hex(const char* input) {
    uint8_t num = 0;

    char c;
    while ((c = *input++) != '\0') {
        num = (num << 4) | _char_to_hex(c); 
    }

    return num;
}


/* 对通信数据包进行日志记录 */
static void print_packet_log(gdb_server_t* gdb_server, int received, char* msg) {
    if (gdb_server->debug_info) {
        char time_buffer[80];
        time_t raw_time;
        time(&raw_time);
        struct tm* time_info = localtime(&raw_time);
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", time_info);
        
        if (received) {
            // 对接收的数据包输出
            printf("%s-<$: %s\n", time_buffer, debug_info_buffer);
        }
        else {
            // 对发送的数据包输出
            printf("%s->$: %s\n", time_buffer, msg);
        }
    }
}

/* 解析 client 发送的数据包 */

static char _read_starter_char(gdb_server_t* gdb_server, char curr) {
    // flags: 以默认方式接收不进行任何特殊处理
    recv(gdb_server->gdb_client, &curr, 1, 0);
    if (curr == EOF) {
        // 没有读到任何数据
        fprintf(stderr, "connection closed by client\n");
        goto close;
    }

    if (curr == '+') {
        // 成功收到数据
        fprintf(stdout, "successfully received data from client!\n");
        curr = EOF;
        return _read_starter_char(gdb_server, curr);
    }

    if (curr == '$') {
        // 成功找到 start_label 返回
        return curr;
    }

close:
    return EOF;
}

static char _read_command_char(gdb_server_t* gdb_server) {
    int curr_index = 0;
    char curr = EOF;
    while(1) {
        if (curr_index > DEBUG_INFO_BUFFER_SIZE) {
            fprintf(stderr, "data oversize\n");
            goto close;
        }

        curr = EOF;
        recv(gdb_server->gdb_client, &curr, 1, 0);

        if (curr == EOF) {
            fprintf(stderr, "connection closed by client\n");
            goto close;
        }

        if (curr == '#') {
            // 判断是否读取到 checksum 段
            break;
        }

        // 根据 command 更新 checksum   (注意在转义之前计算)
        checksum += curr;

        if (curr == GDB_ESCAPE) {
            // 对转义字符的处理     '}#^0x20'
            recv(gdb_server->gdb_client, &curr, 1, 0);  // 读入下一个字符
            curr = curr ^ 0x20;
        }
        
        // 读取 command 有效数据
        debug_info_buffer[curr_index++] = curr;
    }

    // 读取到 checksum 标记结束循环
    debug_info_buffer[curr_index] = '\0';       // 定义为一个完整字符串
    return curr;

close:
    return EOF;
}

static char _read_checksum_char(gdb_server_t* gdb_server) {
    // 类似于 "#ca" 这样 两位 16 进制字符
    
    // 接收读取的 checksum
    char read_checksum_str[3] = {0};
    uint8_t read_checksum_num = 0;

    char curr = EOF;

    // 以字符的方式读取两位
    for (int i = 0; i < 2; i++) {
        curr = EOF;
        recv(gdb_server->gdb_client, &curr, 1, 0);

        if (curr == EOF) {
            fprintf(stderr, "connection closed by client\n");
            goto close;
        }

        read_checksum_str[i] = curr;
    }

    // 将字符结果转换为数字
    read_checksum_num = _str_to_hex(read_checksum_str);

    // 判断 checksum 的合法性
    if (read_checksum_num != checksum) {
        if (gdb_server->debug_info) {
            printf("-> %s\n", debug_info_buffer);
        }

        fprintf(stderr, "checksum failed\n");

        // 通知客户端请求重新发送
        send(gdb_server->gdb_client, "-", 1, 0);
    }
    else {
        // 通知客户端接收成功
        send(gdb_server->gdb_client, "+", 1, 0);
        curr = '0';
    }
    return curr;

close:
    return EOF;
}

/* 生成 server 发送的数据包 */

// 根据数据包格式打包
static int gdb_write_packet(gdb_server_t* gdb_server, char* msg) {
    char reply_buffer[DEBUG_INFO_BUFFER_SIZE];
    int reply_buffer_index = 0;
    char checksum_hex[4] = {0};     // 字符类型的校验和
    uint8_t write_checksum = 0;

    // 定义起始符号
    reply_buffer[reply_buffer_index++] = '$';
    for(char* ptr = msg; *ptr; ptr++) {
        char c = *ptr;

        // 对转义字符进行一个额外的处理
        if (c == '#' || c == '$' || c == '*' || c == '}') {
            reply_buffer[reply_buffer_index++] = GDB_ESCAPE;
            c ^= 0x20;
            write_checksum += GDB_ESCAPE;
        }
        reply_buffer[reply_buffer_index++] = c;
        write_checksum += c;
    }

    // 写入校验和
    // char *const _Buffer: 目标缓冲区
    // size_t size: 表示缓冲区大小  包括终止符 '\0'
    // 格式化字符串并将其写入到一个缓冲区中
    snprintf(reply_buffer + reply_buffer_index, sizeof(write_checksum), "#%02x", write_checksum);

    // 发送数据包
    send(gdb_server->gdb_client, reply_buffer, (int) strlen(reply_buffer), 0);

    // 日志打印
    print_packet_log(gdb_server, 0, msg);

    // 检查对方是否正确接收 (会返回一个 '+')
    char curr = EOF;
    // curr = _read_starter_char(gdb_server->gdb_client, curr);
    // 因为 curr 会在 _read_starter_char 初始化 所以这里没办法用 _read_starter_char() 处理

    recv(gdb_server->gdb_client, &curr, 1, 0);
    RETURN_IF_MSG(curr == EOF, err, "client failed to receive your msg\n");
    
    // 没有正确接收重新发送
    if (curr != '+') {
        RETURN_IF_MSG(curr == EOF, err, "client failed to receive your msg correctly, now retry..\n");
        // 实际上因为项目在本机上而且是 TCP 连接不太可能发生丢包错误
        // 如果是校验码的错误也不会因为重新发包而解决
        // 所以这里只写了框架没有进一步的处理
        goto err;
    }

    if (curr == '+') {
        return 0;
    }
err:
    return -1;
}

static int gdb_write_unsupport(gdb_server_t* gdb_server) {
    // 不支持命令回复       直接返回空字符串
    return gdb_write_packet(gdb_server, "");
}

// 解析 client-socket 发来的数据包内容
static char* gdb_read_packet(gdb_server_t* gdb_server) {
    // 从结构上可以视为三个部分: start_label(+$) + command + '#' + checksum
    // Tip: command 区域包含对 '#' 的转义处理   '}#^0x20'   通过 EOF 判断结束
    
    
    // 因为数据是以字符串的方式传入的 所以定义一个单字符来判断当下读取的字符
    char debug_info_current = EOF;

    while(1) {
        // 对 start_label 的处理
        debug_info_current = _read_starter_char(gdb_server, debug_info_current);

        // 对 command 的处理
        if (debug_info_current != EOF) {
            debug_info_current = _read_command_char(gdb_server);
        }

        // 对 checksum 的处理
        if (debug_info_current == '#') {
            debug_info_current = _read_checksum_char(gdb_server);
            if (debug_info_current == '0') {
                break;
            }
        }
    }

    // 日志打印
    print_packet_log(gdb_server, 1, "");

    // 已经读取一个完整数据包   返回解析字符串
    return debug_info_buffer;
}


// 定义服务器的监听端口
gdb_server_t* gdb_server_create(struct _riscv_t* riscv, int gdb_port, int debug_info) {
    gdb_server_t* gdb_server = (gdb_server_t*) calloc(1, sizeof(gdb_server_t));
    RETURN_IF_MSG(gdb_server == NULL, err, "gdb_server create failed");
    
    // 1. 创建套接字
    // AF_INET: 声明使用 IpV4 的地址格式
    // SOCK_STREAM: 默认使用流式传输
    // protocol: 0 让系统选择合适的协议(在选择了 AF_INET 和 SOCK_STREAM 的情况下默认是 IPPROTO_TCP )
    socket_t gdb_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    RETURN_IF_MSG(gdb_socket == -1, err, strerror(errno))

    // 在多次连续启动时 声明复用上一次的端口
    int reuse = 1;
    int rc = setsockopt(gdb_socket, SOL_SOCKET, SO_REUSEADDR, (void*) &reuse, sizeof(reuse));
    RETURN_IF_MSG(rc < 0, err, strerror(errno))

    // 2. 绑定套接字
    struct sockaddr_in sockaddr;            // socketaddr_in 为 IpV4 设计的 socket 地址结构
    sockaddr.sin_family = AF_INET;
    // 利用常数 INADDR_ANY 分配服务器端的IP地址 可自动获取运行服务器的计算机 IP 地址
    // 在这个项目里直接写 127.0.0.1 也 ok
    // Tip: 由于转换大小端口过程中的补 0 问题   这两个函数不能混用
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);       // htonl(): host to net long        处理 IpV4 地址
    sockaddr.sin_port = htons(gdb_port);                // htons(): host to net short       处理 port
    
    // 3. 开始监听
    // 对 sockaddr_in 强制类型转换为 sockaddr
    rc = bind(gdb_socket, (struct sockaddr*) &sockaddr, sizeof(sockaddr));
    RETURN_IF_MSG(rc < 0, err, strerror(errno))
    
    // 4. 进入监听状态
    // backlog: 指定监听套接字的等待队列的最大长度  | 有多少个请求可以被系统排队接受而不是立刻拒绝
    rc = listen(gdb_socket, 1);
    RETURN_IF_MSG(rc < 0, err, strerror(errno))

    gdb_server->riscv = riscv;
    gdb_server->debug_info = debug_info;
    gdb_server->gdb_socket = gdb_socket;

    return gdb_server;

err:
    exit(-1);
}

// 等待前端的连接请求并建议连接
static void gdb_client_wait(gdb_server_t* gdb_server) {
    struct sockaddr_in client_addr;
    int clinet_addr_size = sizeof(client_addr);

    // 得到与 client 的通信端口
    socket_t client_socket = accept(gdb_server->gdb_socket, (struct sockaddr*) &client_addr, &clinet_addr_size);
    // 同时监听端口不会被占用   可能有其他 client 发来连接请求  防止冲突

    RETURN_IF_MSG(client_socket == INVALID_SOCKET, err, strerror(errno))

    gdb_server->gdb_client = client_socket;     // 后续对 image.bin 文件的操作都通过 client_socket 进行

    return;
err:
    exit(-1);
}

// 通过 gdb 实现 run by step
static void handle_connection(gdb_server_t* gdb_server) {
    char* packet_data;

    // 读取 client 发来的数据包     根据 Remote Serial Protocol 的格式进行解析 (就是词法分析确信)
    while ((packet_data = gdb_read_packet(gdb_server)) != NULL) {
        // 根据 packet_data 的读取结果进行一些反应

        // 但目前就是 也不知道回啥  所以无论是任何命令 都回复不支持 当然后面要改
        gdb_write_unsupport(gdb_server);
    }

}

// 断开连接
static void gdb_client_close(gdb_server_t* gbd_server) {

}

void gdb_server_run(gdb_server_t* gbd_server) {
    // 类似于服务器后端工作
    while(1) {
        // 等待前端 gdb_client 的连接请求
        gdb_client_wait(gbd_server);
        handle_connection(gbd_server);
        gdb_client_close(gbd_server);
    }
}