#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "core/riscv.h"
#include "gdb_server.h"
#include "plat/plat.h"

static char request[GDB_PACKET_SIZE];

/**
 * 将字符转换为16进制
*/
static char ch2hex (char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    } else if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    } else {
        return 0;
    }
}

static int buf2num (const char * buf) {
    int num = 0;

    char c;
    while ((c = *buf++) != '\0') {
        num = num << 4 | ch2hex(c);
    }
    return num;
}

/**
 * 创建一个GDB服务器，用于远程调试RISC-V处理器。
 */
gdb_server_t * gdb_server_create(riscv_t * riscv, int port, int debug) {
    gdb_server_t *server = (gdb_server_t *) calloc(1, sizeof(gdb_server_t));
    RETURN_IF_MSG(server == NULL, err, "malloc failed");

    // create socket
    socket_t sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    RETURN_IF_MSG(sock_fd == -1, err, strerror(errno));

    // 允许在同一端口上立即重新使用
    int reuse = 1;
    int rc = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse));
    RETURN_IF_MSG(rc < 0, err, strerror(errno));

    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(port);
    rc = bind(sock_fd, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
    RETURN_IF_MSG(rc < 0, err, strerror(errno));

    rc = listen(sock_fd, 1);
    RETURN_IF_MSG(rc < 0, err, strerror(errno));

    server->riscv = riscv;
    server->sock = sock_fd;
    server->debug = debug;
    return server;
err:
    exit(-1);
    return NULL;
}

/**
 * 等待客户端的连接。
*/
static int gdb_wait_client (gdb_server_t * server) {
    // 等待客户端连接
    struct sockaddr_in sockaddr;
    int socklen = sizeof(sockaddr);
    socket_t client = accept(server->sock, (struct sockaddr *) &sockaddr, &socklen);
    RETURN_IF_MSG(client == INVALID_SOCKET, err, strerror(errno));

    // 记录读取信息，转换成文件描述符方便进行处理
    server->client = client;
    return 0;
err:
    exit(-1);
    return -1;
}

/**
 * 关闭GDB连接
 */
static void gdb_close_connection (gdb_server_t * server) {
#ifdef _WIN32
    closesocket(server->client);
#else
    close(server->client);
#endif
}

/**
 * 读取GDB数据包:$packet-data#checksum
 */
static char * gdb_read_packet (gdb_server_t * server) {

    while (1) {
         // 找到第数据包开始$
        char ch = EOF;
        recv(server->client, &ch, 1, 0);
        if (ch == EOF) {
            fprintf(stderr, "connection closed by remote.\n");
            goto err;
        } else if (ch != '$') {
            fprintf(stderr, "skip: %c\n", ch);
            continue;
        }

        // 读取数据部分
        int count = 0;
        uint8_t checksum = 0;
        while (1) {
            if (count >= GDB_PACKET_SIZE) {
                fprintf(stderr, "packet size too large.\n");
                goto err;
            }

            ch = EOF;
            recv(server->client, &ch, 1, 0);
            RETURN_IF_MSG(ch == EOF, err, "connection closed by remote.\n");
            if (ch == '#') {
                break;
            }
            checksum += ch;

            // 如果是转义字符，跳过去取后面的字符
            if (ch == GDB_ESCAPE) {
                ch = EOF;
                recv(server->client, &ch, 1, 0);
                RETURN_IF_MSG(ch == EOF, err, "connection closed by remote.\n");
                checksum += ch;
                ch ^= 0x20;
           }
            request[count++] = ch;
        }
        request[count] = '\0';

        // 读取校验码
        char temp[3] = {0};
        uint8_t read_checksum = 0;
        for (int i = 0; i < 2; i++) {
            ch = EOF;
            recv(server->client, &ch, 1, 0);
            RETURN_IF_MSG(ch == EOF, err, "connection closed by remote.\n");
            temp[i] = ch;
        }
        read_checksum = buf2num(temp);

        // 检查校验码
        if (read_checksum != checksum) {
            if (server->debug) {
                printf("-> %s\n", request);
            }
            fprintf(stderr, "checksum error: recv(0x%02x) != cal(0x%02x)\n", read_checksum, checksum);
            send(server->client, "-", 1, 0);
            goto err;
        } else {
            // 通知成功接收
            send(server->client, "+", 1, 0);
        }
        break;
    }

    // 通信日志输出
    if (server->debug) {
        char buffer[80];
        time_t rawtime;
        time(&rawtime);
        struct tm * info = localtime(&rawtime);
        strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", info);
        printf("%s ->$%s\n", buffer, request);
    }
    return request;
err:
    return NULL;
}

/**
 * 发送数据包: $packet-data#checksum
 */
static int gdb_write_packet(gdb_server_t * server, char *data) {
    char tx_buf[1024];
    int idx = 0;

    // 数据起始
    tx_buf[idx++] = '$';

    uint8_t checksum = 0;
    for (char * ptr = data; *ptr != 0; ptr++) {
        char c = *ptr;

        if (c == EOF) {
            break;

        }

        // 字符转义
        if ((c == '$') || (c == '#') || (c == GDB_ESCAPE) || (c == '*')) {
            char esc = GDB_ESCAPE;
            tx_buf[idx++] = esc;

            checksum += esc;
            *ptr ^= 0x20;
        }

        tx_buf[idx++] = c;
        checksum += *ptr;
    }

    // 写校验和部分
    char hex[4] = {0};
    snprintf(tx_buf + idx, sizeof(hex), "#%02x", checksum);
    send(server->client, tx_buf, (int)strlen(tx_buf), 0);

    // 日志打印
    if (server->debug) {
        char buffer[80];
        time_t rawtime;
        time(&rawtime);
        struct tm * info = localtime(&rawtime);
        strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", info);
        printf("%s <-$%s\n", buffer, data);
    }

    // 读取响应
    char ch = EOF;
    recv(server->client, &ch, 1, 0);
    RETURN_IF_MSG(ch == EOF, err, "connection closed by remote.");
    if (ch != '+') {
        fprintf(stderr, "not +\n");
    }

    return 0;
err:
    return -1;
}

/**
 * 发送不支持的命令: $#00
*/
static int gdb_write_unsupport (gdb_server_t * server) {
    return gdb_write_packet(server, "");
}

/**
 * 发送停止数据包
*/
static int gdb_write_stop (gdb_server_t * server) {
    snprintf(server->tx_buf, sizeof(server->tx_buf), "S%02x", GDB_SIGTRAP);
    return gdb_write_packet(server, server->tx_buf);
}

/**
 * 发送错误码：E NN
*/
static int gdb_write_error (gdb_server_t * server, int code) {
    sprintf(server->tx_buf, "E%02x", code);
    return gdb_write_packet(server, server->tx_buf);
}

/**
 * 查询状态
*/
static int gdb_handle_query (gdb_server_t * server, char * packet) {
    if (strncmp(packet, "Supported", 9) == 0) {
        snprintf(server->tx_buf, sizeof(server->tx_buf), "PacketSize=%x;vContSupported+", GDB_PACKET_SIZE);
        return gdb_write_packet(server, server->tx_buf);
    } else if (strncmp(packet, "Attached", 9) == 0) {
        // 检查是否已经连接到了被调试对像
        snprintf(server->tx_buf, sizeof(server->tx_buf), "1");
        return gdb_write_packet(server, server->tx_buf);
    } else if (strcmp(packet, "Rcmd,726567") == 0) {
        // 复位RISC-V内核
        riscv_reset(server->riscv);

        snprintf(server->tx_buf, sizeof(server->tx_buf), "OK");
        return gdb_write_packet(server, server->tx_buf);
    } else if (strncmp(packet, "Rcmd", 4) == 0) {
        snprintf(server->tx_buf, sizeof(server->tx_buf), "OK");
        return gdb_write_packet(server, server->tx_buf);
    }

    return gdb_write_unsupport(server);
}

/**
 * 在第一次建立GDB连接时，查询目标是什么原因停下的
*/
static int gdb_handle_query_stop (gdb_server_t * server, char * packet) {
    return gdb_write_stop(server);
}

static char * write_mem_hex (char * buf, riscv_word_t data, int size) {
    for (int i = 0; i < size; i++) {
        char c = (data & 0xF0) >> 4;
        *buf++ = c < 10 ? ('0' + c) : ('a' + c - 10);
        c = data & 0xF;
        *buf++ = c < 10 ? ('0' + c) : ('a' + c - 10);
        data >>= 8;
    }
    return buf;
}

/**
 * 批量读取通用寄存器命令处理:g
 * 注意，这里使用GDB内部的默认寄存器布局
 */
static int gdb_handle_read_regs (gdb_server_t * server, char * packet) {
    char * tx_buf = server->tx_buf;
    for (int i = 0; i < RISCV_REGS_SIZE; i++) {
        tx_buf = write_mem_hex(tx_buf, riscv_read_reg(server->riscv, i), 4);
    }

    *tx_buf = '\0';
    return gdb_write_packet(server, server->tx_buf);
}

/**
 * 读取指定的寄存器: p n
*/
static int gdb_handle_read_reg (gdb_server_t * server, char * packet) {
    // 寄存器序号
    char * token = strtok(packet, "=");
    RETURN_IF_MSG(token == NULL, err, "packet error");
    int reg = strtoul(packet, NULL, 16);
    if (reg > RISCV_REGS_SIZE) {
        return gdb_write_error(server, 1);
    }
    
    riscv_word_t data;
    if (reg < RISCV_REGS_SIZE) {
        data = riscv_read_reg(server->riscv, reg);
    } else if (reg == RISCV_REGS_SIZE) {
        data = server->riscv->pc;
    }
    char * tx_buf = write_mem_hex(server->tx_buf, data, sizeof(riscv_word_t));
    *tx_buf = '\0';
    return gdb_write_packet(server, server->tx_buf);
err:
    return gdb_write_error(server, -1);
}

/**
 * 读取内存命令处理: m addr,length
 */
static int gdb_handle_read_mem(gdb_server_t * server, char * packet) {
    // 解析地址和长度
    char * token = strtok(packet, ",");
    RETURN_IF_MSG(token == NULL, err, "packet error");
    int addr = buf2num(token);

    token = strtok(NULL, ",");
    RETURN_IF_MSG(token == NULL, err, "packet error");
    int length = buf2num(token);

    char * read_buf = (char *)calloc(1, length);
    riscv_mem_read(server->riscv, addr, (uint8_t *)read_buf, length);

    char * tx_buf = server->tx_buf;
    for (int i = 0; i < length; i++) {
        tx_buf = write_mem_hex(tx_buf, read_buf[i], 1);
    }
    free(read_buf);

    *tx_buf = '\0';
    gdb_write_packet(server, server->tx_buf);
    return 0;
err:
    gdb_write_error(server, 01);
    return -1;
}

/**
 * 写内存命令处理: m addr,length:xx...
 */
static int gdb_handle_write_mem(gdb_server_t * server, char * packet) {
    // 解析地址和长度
    char * token = strtok(packet, ",");
    RETURN_IF_MSG(token == NULL, err, "packet error");
    int addr = buf2num(token);

    token = strtok(NULL, ":");
    RETURN_IF_MSG(token == NULL, err, "packet error");
    int length = buf2num(token);

    // 取数据并写入
    token = strtok(NULL, ":");
    char * tx_buf = server->tx_buf;
    for (int i = 0; (i < length) && *token; i++) {
        char data = ch2hex(*token++) << 4;
        if (*token == '\0') {
            break;
        }
        data |= ch2hex(*token++);
        riscv_mem_write(server->riscv, addr + i, &data, 1);
    }

    gdb_write_packet(server, "OK");
    return 0;
err:
    gdb_write_error(server, 01);
    return -1;
}

/**
 * 单步执行命令: s [addr]
*/
static int gdb_handle_step (gdb_server_t * server, char * packet) {
    riscv_continue(server->riscv, 0);
    return gdb_write_stop(server);
}

/**
 * continue全速运行: c [addr]
 */
static int gdb_handle_continue(gdb_server_t * server, char * packet) {
    // 带地址的不支持
    if (packet[0] !='\0') {
        return gdb_write_unsupport(server);
    }

    unsigned long ul=1;
    int ret = ioctlsocket(server->client, FIONBIO,(unsigned long *)&ul);
    if(ret==SOCKET_ERROR) {
        printf("error");
    }
    riscv_continue(server->riscv, 1);

    ul = 0;
    ioctlsocket(server->client, FIONBIO,(unsigned long *)&ul);

    return gdb_write_stop(server);
}

/**
 * 取消断点：z type,addr,kind
*/
static int gdb_handl_remove_breakpoint(gdb_server_t * server, char * packet) {
    // 类型
    char * token = strtok(packet, ",");
    RETURN_IF_MSG(token == NULL, err, "packet error");
    int type = buf2num(token);

    // 只支持硬件断点
   if ((type == 1) || (type == 0)) {
        // 地址
        token = strtok(NULL, ",");
        RETURN_IF_MSG(token == NULL, err, "packet error");
        uint32_t addr = buf2num(token);

        // 忽略kind
        riscv_remove_breakpoint(server->riscv, addr);
        return gdb_write_packet(server, "OK");
    } else {
        return gdb_write_unsupport(server);
    }
err:
    return gdb_write_error(server, 01);
}

/**
 * 设置断点：Z type,addr,kind
*/
static int gdb_handle_set_breakpoint (gdb_server_t * server, char * packet) {
    // 类型
    char * token = strtok(packet, ",");
    RETURN_IF_MSG(token == NULL, err, "packet error");
    int type = buf2num(token);

    // 只支持硬件断点
   if ((type == 1) || (type == 0)) {
        // 地址
        token = strtok(NULL, ",");
        RETURN_IF_MSG(token == NULL, err, "packet error");
        uint32_t addr = buf2num(token);

        // 忽略kind
        riscv_add_breakpoint(server->riscv, addr);
        return gdb_write_packet(server, "OK");
    } else {
        return gdb_write_unsupport(server);
    }
err:
    return gdb_write_error(server, 01);
}

/**
 * 处理GDB连接请求
 */
static void handle_connection (gdb_server_t * server) {
    riscv_reset(server->riscv);

    char * packet;
    while ((packet = gdb_read_packet(server)) != NULL) {
        char ch = *packet++;
        switch (ch) {
            case 'q':
                gdb_handle_query(server, packet);
                break;
            case 's':
                gdb_handle_step(server, packet);
                break;
            case 'c':
                gdb_handle_continue(server, packet);
                break;
            case 'g':
                gdb_handle_read_regs(server, packet);
                break;
            case 'm':
                gdb_handle_read_mem(server, packet);
                break;
            case 'M':
                gdb_handle_write_mem(server, packet);
                break;
            case 'p':
                gdb_handle_read_reg(server, packet);
                break;
            case 'z':
                gdb_handl_remove_breakpoint(server, packet);
                break;
            case 'Z':
                gdb_handle_set_breakpoint(server, packet);
                break;
            case '?':
                gdb_handle_query_stop(server, packet);
                break;
                break;
            case 'k':
                return;
            case 'D':
                // GDB断开连接了
                return;
            default:
                gdb_write_unsupport(server);
                break;
        }
    }
}

/**
 * GDB服务运行
*/
void gdb_server_run(gdb_server_t *server) {
    while (1) {
        gdb_wait_client(server);
        handle_connection(server);
        gdb_close_connection(server);
    }
}
