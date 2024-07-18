#include <stdlib.h>
#include <string.h>

#include "core/riscv.h"
#include "plat/plat.h"          // 包含对应环境中的 BSD Socket 初始化

// Q: 网络字节序与主机字节序的区别

gdb_server_t* gdb_server_create(struct _riscv_t* riscv, int gdb_port, int debug_info) {
    gdb_server_t* gdb_server = (gdb_server_t*) calloc(1, sizeof(gdb_server_t));
    RETURN_IF_MSG(gdb_server == NULL, err, "gdb_server create failed");
    
    // 1. 创建套接字
    // AF_INET: 声明使用 IpV4 的地址格式
    // SOCK_STREAM: 默认使用流式传输
    // protocol: 0 让系统选择合适的协议(在选择了 AF_INET 和 SOCK_STREAM 的情况下默认是 IPPROTO_TCP )
    socket_t gdb_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    RETURN_IF_MSG(gdb_socket == NULL, err, strerror(errno))

    // 在多次连续启动时 声明复用上一次的端口
    int reuse = 1;
    int rc = setsockopt(gdb_socket, SOL_SOCKET, SO_REUSEADDR, (void*) &reuse, sizeof(reuse));
    RETURN_IF_MSG(gdb_socket == NULL, err, strerror(errno))

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
static void gdb_client_wait(gdb_server_t* gbd_server) {

}

// 通过 gdb 实现 run by step
static void handle_connection(gdb_server_t* gbd_server) {

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