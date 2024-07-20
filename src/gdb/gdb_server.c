#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "core/riscv.h"
#include "plat/plat.h"          // 包含对应环境中的 BSD Socket 初始化

// 定义 command 全局缓存
static char debug_info_buffer[DEBUG_INFO_BUFFER_SIZE];      // 全部初始化为 0

// 定义 command-checksum
static uint8_t checksum = 0;

/* 芯片模拟默认小端方式 */

/* 辅助函数 */

// 把 checksum-str 转换为 checksum-num
static char _char_to_hex(char c) {
    if ((c >= '0') && (c <= '9'))
        return c - '0';
    if ((c >= 'a') && (c <= 'f'))
        return c - 'a' + 10;
    if ((c >= 'A') && (c <= 'F'))
        return c - 'A' + 10;
    return 0;
}

static uint8_t _str_to_hex(const char* input) {
    uint8_t num = 0;

    char c;
    while ((c = *input++) != '\0') {
        num = (num << 4) | _char_to_hex(c); 
    }

    return num;
}

// 把寄存器中的内容写到特定位置的内存中
static char* write_mem_from_reg(char* target_mem, riscv_word_t source_reg, int size) {
    // 根据 size 的大小 每个字节每个字节的去写入
    for (int i = 0; i < size ; i ++) {
        // 以小端的方式发送     0x12345678 => 78 56 34 12

        // 取出字节中的倒数第二位
        char c = (source_reg & 0xF0) >> 4;
        *target_mem++ = (c < 10) ? ('0' + c) : (c - 'a' + 10);

        // 取出字节中的最后一位
        c = (source_reg & 0xF);
        *target_mem++ = (c < 10) ? ('0' + c) : (c - 'a' + 10);

        // 更新寄存器内容
        source_reg = (source_reg >> 8);
    }

    return target_mem;
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
            checksum += curr;
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
    // format: (#) 使用替代形式 针对 '%x' / '%X' 的添加 0x / 0X   (%) 每个格式说明符都以 '%' 开始    (0) 零填充   (2) 指定宽度   (x) 十六进制输出整数
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

    recv(gdb_server->gdb_client, &curr, 1, 0);  // Q: 卡死
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

// 发送一个声明错误的数据包
static int gdb_write_err_packet(gdb_server_t* gdb_server, int code) {
    sprintf(gdb_server->gdb_send_buffer, "E%02x", code);
    return gdb_write_packet(gdb_server, gdb_server->gdb_send_buffer);
}

static int gdb_write_unsupport(gdb_server_t* gdb_server) {
    // 不支持命令回复       直接返回空字符串
    return gdb_write_packet(gdb_server, "");
}

/* 根据 gdb_command 对模拟器的调试操作 */

static int gdb_read_regs(gdb_server_t* gdb_server) {
    // 32 个寄存器的内容按顺序拼接在一起    以 16 进制的方式发送

    char* reg_buffer = gdb_server->gdb_send_buffer;
    for (int i = 0; i < RISCV_REG_NUM; i ++) {
        // 按顺序读取寄存器的值并且每次写入 4B
        reg_buffer = write_mem_from_reg(reg_buffer, riscv_read_reg(gdb_server->riscv, i), 4);
        // 更新目前记录数据的指针指向
    }

    // 把读取到的完整消息设置为一个字符串
    *reg_buffer = '\0';
    
    // 发送数据包
    return gdb_write_packet(gdb_server, gdb_server->gdb_send_buffer);
}

static int gdb_read_specified_reg(riscv_t* riscv, int reg_num) {
    return riscv_read_reg(riscv, reg_num);
}

/* 根据 gdb 协议生成控制命令 */

static int gdb_singal_stop(gdb_server_t* gdb_server) {
    // S05: 表示目标由于一个 trap 指令停止
    snprintf(gdb_server->gdb_send_buffer, DEBUG_INFO_BUFFER_SIZE, "PacketSize=%x;vContSupported+", DEBUG_INFO_BUFFER_SIZE);
    return gdb_write_packet(gdb_server, gdb_server->gdb_send_buffer);
}

static int gdb_handle_command_q(gdb_server_t* gdb_server, char* packet_data) {
    if (strncmp(packet_data, "Supported", 9) == 0) {
        // GDB-client 查询目标所支持的功能      目前响应定义为支持 vContSupported 功能
        snprintf(gdb_server->gdb_send_buffer, DEBUG_INFO_BUFFER_SIZE, "PacketSize=%x;vContSupported+", DEBUG_INFO_BUFFER_SIZE);
        return gdb_write_packet(gdb_server, gdb_server->gdb_send_buffer);
    }

    if (strncmp(packet_data, "?", 1) == 0) {
        // 查询目标为何停止     1/断点  2/信号  3/异常 ...
        return gdb_singal_stop(gdb_server);
    }

    if (strncmp(packet_data, "Attached", 8) == 0) {
        // 查询 gdb 是否是以附加的方式连接到目标
        // 附加方式: 程序并非由 gdb 启动    通过 pid 执行
        // 启动方式: 由 gdb 启动该程序
        // 模拟器是单独的程序 通过通信的方式调试 所以是附加方式
        snprintf(gdb_server->gdb_send_buffer, DEBUG_INFO_BUFFER_SIZE, "1");
        return gdb_write_packet(gdb_server, gdb_server->gdb_send_buffer);
    }

    /* 下面是不支持的命令 其实可以用统一的用 unsupport 表示 但是写出来更清晰一些 */

    if(strncmp(packet_data, "TStatus", 7) == 0) {
        // 查询目标的追踪状态   通常在追踪点功能时使用  和 collect 配合使用 捕获特定点的信息而不暂停执行
        return gdb_write_unsupport(gdb_server);
    }

    if(strncmp(packet_data, "fThreadInfo", 11) == 0) {
        // 请求第一批线程的信息
        return gdb_write_unsupport(gdb_server);
    }

    if(strncmp(packet_data, "C", 1) == 0) {
        // 请求当前线程的 ID
        return gdb_write_unsupport(gdb_server);
    }
    return gdb_write_unsupport(gdb_server);
}

static int gdb_handle_command_p(gdb_server_t* gdb_server, char* packet_data) {
    // 获取指定的寄存器编号
    int reg_num = strtoul(packet_data, &packet_data, 16);

    // 判断编号是否合法
    if (reg_num > RISCV_REG_NUM) {
        // 发送命令错误数据包
        return gdb_write_err_packet(gdb_server, 1);
    }
    
    // 得到指定寄存器内容   x0 - x31 | pc
    riscv_word_t reg_data;
    if (reg_num < RISCV_REG_NUM) {
        // 普通寄存器读取
        reg_data = gdb_read_specified_reg(gdb_server->riscv, reg_num);
    }

    if (reg_num == RISCV_REG_NUM) {
        // pc 读取
        reg_data = gdb_server->riscv->pc;
    }

    // 写入指定内存地址
    char* mem_buffer = write_mem_from_reg(gdb_server->gdb_send_buffer, reg_data, 4);
    *mem_buffer++ = '\0';

    // 发送数据包
    return gdb_write_packet(gdb_server, gdb_server->gdb_send_buffer);
}

static int gdb_handle_command_g(gdb_server_t* gdb_server, char* packet_data) {
    // 请求所有寄存器的值   全0: 寄存器清空或者未运行任何代码
    // 在模拟器返回的时候 32 * 32 bit
    return gdb_read_regs(gdb_server);
}

static int gdb_handle_command_v(gdb_server_t* gdb_server, char* packet_data) {
    // 测试命令 确认目标可以正确响应即使没有有效数据的命令      响应定义为空
    if (strncmp(packet_data, "MustReplyEmpty", 14) == 0) {
        return gdb_write_unsupport(gdb_server);
    }
    return gdb_write_err_packet(gdb_server, 2);
}

static int gdb_handle_command_h(gdb_server_t* gdb_server, char* packet_data) {
    // 线程相关的命令   不过当下的环境用不到 所以直接返回空
    // Hg0: 选择哪些线程是活动的线程     Hg-1: 在所有线程上执行调试命令
    // Hc-1: 所有线程都被设置为被调试的 => 暂停所有线程
    return gdb_write_unsupport(gdb_server);
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

        // 建立调试状态     根据 gdb_P793 的回复格式定义
        char curr = *packet_data++;
        switch (curr) {
            case 'q':
                gdb_handle_command_q(gdb_server, packet_data);
                break;
            
            case 'v':
                gdb_handle_command_v(gdb_server, packet_data);
                break;

            case 'H':
                gdb_handle_command_h(gdb_server, packet_data);
                break;

            case '?':
                gdb_handle_command_q(gdb_server, packet_data);
                break;

            case 'g':
                gdb_handle_command_g(gdb_server, packet_data);
                break;

            case 'p':
                gdb_handle_command_p(gdb_server, packet_data);
                break;

            default:
                gdb_write_unsupport(gdb_server);
                break;
        }
        // -q: are general query packets    期望获取支持调试功能    gdb_p808

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