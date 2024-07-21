#include <string.h>
#include "core/riscv.h"
#include "plat/plat.h"
#include "test/instr_test.h"

#define RISCV_RAM_BASE                  0x20000000          // 缺省RAM起始
#define RISCV_RAM_SIZE                  (16*1024*1024)      // 缺省RAM大小
#define RISCV_FLASH_BASE                0x00000000          // 缺省FLASH起始
#define RISCV_FLASH_SIZE                (16*1024*1024)      // 缺省Flash大小

// 显示帮助信息
static void print_usage(const char* filename) {
    fprintf(stderr,
        "usage: %s [options] <elf file>\n"
        "-r addr:size        | set ram address and size(kb)\n"
        "-f addr:size        | set flash address and size(kb)\n"
        "-t                   | Print execution trace\n"
        "-g [port]            | enable gdb stub\n",
        filename
    );
}
int main(int argc, char** argv) {
    int has_ram = 0, has_flash = 0;
    int use_gdb = 0, gdb_port = 1234, gdb_trace = 0;
    int run_test = 0;

    plat_init();

    riscv_t* riscv = riscv_create();

    int arg_idx = 1;
    while (arg_idx < argc) {
        char* arg = argv[arg_idx++];

        if (strcmp(arg, "-h") == 0) {
            print_usage(argv[0]);
            exit(-1);
        } else if (strcmp(arg, "-u") == 0) {
            run_test = 1;
        }  else if (strcmp(arg, "-t") == 0) {
            gdb_trace = 1;
        } else if (strcmp(arg, "-r") == 0) {
            char* options = argv[arg_idx++];
            if (options == NULL) {
                fprintf(stderr, "invalid ram option");
                exit(-1);
            }

            char* token = strtok(options, ":");
            uint32_t addr = strtoul(token, NULL, 16);
            token = strtok(NULL, ":");
            uint32_t size = strtoul(token, NULL, 16);
            mem_t* mem = mem_create("ram", addr, size, RISCV_MEM_ATTR_WRITABLE | RISCV_MEM_ATTR_READABLE);
            riscv_add_device(riscv, (device_t*)mem);
            has_ram = 1;
        } else if (strcmp(arg, "-f") == 0) {
            char* options = argv[arg_idx++];
            if (options == NULL) {
                fprintf(stderr, "invalid flash option");
                exit(-1);
            }

            char* token = strtok(options, ":");
            uint32_t addr = strtoul(token, NULL, 16);
            token = strtok(NULL, ":");
            uint32_t size = strtoul(token, NULL, 16);
            mem_t* mem = mem_create("flash", addr, size, RISCV_MEM_ATTR_WRITABLE);
            riscv_add_device(riscv, (device_t*)mem);
            riscv_set_flash(riscv, mem);
            has_flash = 1;
        } else if (strcmp(arg, "-g") == 0) {
            char* options = argv[arg_idx++];
            if (options) {
                gdb_port = strtoul(options, NULL, 10);
            }
            use_gdb = 1;
        }
    }

    // 没有RAM, 创建缺省的
    if (!has_ram) {
        mem_t* mem = mem_create("ram", RISCV_RAM_BASE, RISCV_RAM_SIZE, RISCV_MEM_ATTR_WRITABLE | RISCV_MEM_ATTR_READABLE);
        riscv_add_device(riscv, (device_t*)mem);
    }

    // 没有Flash, 创建缺省的
    if (!has_flash) {
        mem_t* mem = mem_create("flash", RISCV_FLASH_BASE, RISCV_FLASH_SIZE, RISCV_MEM_ATTR_WRITABLE | RISCV_MEM_ATTR_READABLE);
        riscv_add_device(riscv, (device_t*)mem);
        riscv->flash = mem;
    }

    if (run_test) {
        instr_test(riscv);
        return 0;
    }

    // 是否使用GDB
    if (use_gdb) {
        gdb_server_t* server = gdb_server_create(riscv, gdb_port, gdb_trace);
        if (server == NULL) {
            fprintf(stderr, "create gdb server failed.\n");
            exit(-1);
        }

        printf("gdb is running on port: %d\n", gdb_port);
        riscv->server = server;
    }

    riscv_run(riscv);
    return 0;
}
