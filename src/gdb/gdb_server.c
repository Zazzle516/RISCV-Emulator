#include <stdlib.h>

#include "core/riscv.h"

gdb_server_t* gdb_server_create(struct _riscv_t* riscv, int gdb_port, int debug_info) {
    gdb_server_t* gdb_server = (gdb_server_t*) calloc(1, sizeof(gdb_server_t));
    RETURN_IF_MSG(gdb_server == NULL, err, "gdb_server create failed");
    return gdb_server;

err:
    exit(-1);
}