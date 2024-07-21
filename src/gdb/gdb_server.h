#ifndef GDB_SERVER_H
#define GDB_SERVER_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "plat/plat.h"

#define GDB_PACKET_SIZE          (30*1024)      // 最大接收包大小
#define GDB_ESCAPE               0x7D           // 转义字符
#define GDB_SIGTRAP              0x05          // 停止原因

// 错误处理
#define RETURN_IF_MSG(x, err, msg) \
    if (x) {    \
        fprintf(stderr, "error: %s: %s(%d): %s\n", __FILE__, __FUNCTION__, __LINE__, msg); \
        goto err; \
    }
    
struct _riscv_t;
typedef struct _gdb_server_t {
    struct _riscv_t * riscv;    // CPU

    socket_t sock;              // 服务端socket
    socket_t client;            // 客户端fd
    int debug;                  // 显示gdb运行信息

    char tx_buf[GDB_PACKET_SIZE];    // 发送缓存
}gdb_server_t;

gdb_server_t * gdb_server_create(struct _riscv_t * riscv, int port, int debug);
void gdb_server_run(gdb_server_t *server);

#endif // GDB_SERVER_H