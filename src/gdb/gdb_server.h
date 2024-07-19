#ifndef GDB_SERVER_H
#define GDB_SERVER_H

#include <stdio.h>

#include "plat/plat.h"

#define DEBUG_INFO_BUFFER_SIZE          (30 * 1024)
#define GDB_ESCAPE                      '}'

// 避免头文件嵌套 使用前向定义
struct _riscv_t;

// 如果 expr_x 为真 那么函数跳转到 err 执行并且打印 MSG 错误信息
// 使用 C 语言的预定义跟踪调试
#define RETURN_IF_MSG(x, err, MSG) \
    if (x) {            \
        fprintf(stderr, "error: %s %s(%d): %s\n", __FILE__, __FUNCTION__, __LINE__, MSG);   \
        goto err;       \
    }

typedef struct _gdb_server_t
{
    struct _riscv_t* riscv;     // 声明该 gdb_server 所属的 riscv 对象

    int debug_info;

    socket_t gdb_socket;        // 监听端口
    socket_t gdb_client;        // 通信端口     因为当前的环境只有一个 client 如果是多个可能用数组链表之类的

}gdb_server_t;

// 定义 gdb_server 的创建
// debug_info: 是否在 terminal 打印 debug 信息
gdb_server_t* gdb_server_create(struct _riscv_t* riscv, int gdb_port, int debug_info);

void gdb_server_run(gdb_server_t* gbd_server);

#endif /* GDN_SERVER_H */