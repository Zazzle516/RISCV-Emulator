#include <string.h>
#include "plat/plat.h"
#include <stdio.h>

// 注意，如果包含了instr_test.h，就不要再包含test.h，要去掉，不然可能有冲突
// #include "test.h"

// 模拟器的概念: 运行的是一个大型程序 只是模拟了 RISCV 的行为 所有调用的相关变量和物理资源都是我自己的电脑

// Tip: 尽量不要使用 VSCode 提供的 build 工具   而是直接使用 CLI-cmake 不然缓存不会被更新
// 会反复的出现 LNK2019 的报错  删掉 build-folder 重新构建
#include "core/instr_implements.h"
#include "test/instr_test.h"

// 定义命令行参数的语法
// riscv-sim -p 1234 -ram 0:xxx -flash 0:xxx
static void print_help_info(const char* file_name) {
    fprintf(stderr,
        "usage: %s [options] <elf file>\n"
        "-help              | print help info\n"
        "-test              | instructions unit tests\n"
        "-debug port        | run step by step and it is optional to define your debug info port\n"
        "-ram start:size    | set start addres of RAM and size\n"
        "-flash start:size  | set start address of Flash and size\n"
        "-info              | print debug info on terminal\n"
        ,file_name
    );
}

static void arg_check(char* args) {
    if (args == NULL) {
        fprintf(stderr, "Empty arg input\n");
        exit(-1);
    }
}

#define RISCV_FLASH_START               0                       // 现在很多芯片地址都是从零开始
#define RISCV_FLASH_SIZE                (16 * 1024 * 1024)

#define RISCV_RAM_START                 0x20000000              // 根据测试工程决定在这个位置
#define RISCV_RAM_SIZE                  (16 * 1024 * 1024)

int main(int argc, char** argv) {
    plat_init();

    // 注意，去掉test_env后，注意要把test.h也给去掉
    // test_env();

    // 创建模拟器对象
    riscv_t* myRiscv = riscv_create();

    // 通过 argv 对命令行传参进行遍历
    int arg_index = 1;

    // 用户是否使用自定义参数的标志
    int ram_define_flag = 0;
    int flash_define_flag = 0;
    int run_test_flag = 0;
    int default_debug_port = 1234;
    int debug_mode = 0;                 // 默认不开启
    int print_debug_info = 1;

    while(arg_index < argc) {
        char* currArg = argv[arg_index++];
        // *argv[arg_index] 这样应该是直接把内容取出来了吧

        if (strcmp(currArg, "-ram") == 0) {
            // 定义 RAM 的地址和大小
            char* ram_args = argv[arg_index++];
            arg_check(ram_args);

            // 利用 ":" 分割        strtok(): 将字符串分解为一组字符串
            char* ram_define = strtok(ram_args, ":");
            
            // 注意结果是字符串 需要转换为数字
            uint32_t ram_start_addr = strtoul(ram_define, NULL, 16);

            // 通过 strtok() 得到第二个参数
            // strtok() 会在内部维护一个状态    (这个函数还挺奇妙的...
            ram_define = strtok(NULL, ":");
            uint32_t ram_size = strtoul(ram_define, NULL, 16);

            // 创建 RAM 空间
            // 通过 或操作 完成读写操作的同时定义
            // 更新为用户自定义的起始地址和大小
            mem_t* myRAM = mem_create("ram", RISCV_MEM_ATTR_READABLE | RISCV_MEM_ATTR_WRITABLE, ram_start_addr, ram_size);
            // 不要求外部设备类型一定是 Memory 所以是通过 device_t 结构体去控制  要进行强制类型转换
            // (riscv_device_t* myRAM)  也可以  只是挂了一个控制体上去  省区了 uint8_t* 的指针
            riscv_device_add(myRiscv, &myRAM->riscv_dev);

            // 更新自定义标志
            ram_define_flag = 1;
        }

        if (strcmp(currArg, "-flash") == 0) {
            // 流程与 "-ram" 类似
            char* flash_args = argv[arg_index++];
            arg_check(flash_args);

            char* flash_define = strtok(flash_args, ":");
            uint32_t flash_start_addr = strtoul(flash_define, NULL, 16);
            flash_define = strtok(NULL, ":");
            uint32_t flash_size = strtoul(flash_define, NULL, 16);

            mem_t* myMemory = mem_create("flash", RISCV_MEM_ATTR_READABLE, flash_start_addr, flash_size);
            riscv_device_add(myRiscv, (riscv_device_t*) myMemory);
            riscv_flash_set(myRiscv, myMemory);

            flash_define_flag = 1;
        }

        if (strcmp(currArg, "-help") == 0) {
            print_help_info(argv[0]);
            exit(0);
        }

        if (strcmp(currArg, "-test") == 0) {
            // 避免直接执行程序 可能还有 -ram / -flash 参数未定义
            run_test_flag = 1;
        }

        if (strcmp(currArg, "-debug") == 0) {
            // 只有要求以 debug 方式编译的时候才保存 gdb 信息 提高效率
            // 用户自定义端口只有处于 debug 状态的时候才有意义  所以把 port 定义在里面
            char* option_port = argv[arg_index++];
            if (option_port) {
                // 如果用户自定义端口号存在的话
                default_debug_port = strtoul(option_port, NULL, 10);
            }
            debug_mode = 1;
        }

        if (strcmp(currArg, "-info") == 0) {
            print_debug_info = 1;
        }
    }

    // 判断一下是否使用默认 RAM 参数
    if (ram_define_flag == 0) {
        mem_t* myRAM = mem_create("ram", RISCV_MEM_ATTR_READABLE | RISCV_MEM_ATTR_WRITABLE, RISCV_RAM_START, RISCV_RAM_SIZE);
        riscv_device_add(myRiscv, &myRAM->riscv_dev);
    }

    // 创建 Flash 空间
    if (flash_define_flag == 0) {
        mem_t* myMemory = mem_create("flash", RISCV_MEM_ATTR_READABLE, RISCV_FLASH_START, RISCV_FLASH_SIZE);

        // 挂载到模拟器中
        riscv_device_add(myRiscv, (riscv_device_t*) myMemory);
        riscv_flash_set(myRiscv, myMemory);
    }

    // 读取 image.bin 文件到 Flash 空间
    // flash_load_bin(myRiscv, "./unit/ebreak/obj/image.bin");
    // riscv_reset(myRiscv);
    // fprintf(stdout, "myRiscv has reset with pc = %d", myRiscv->pc);

    // 代码框架实际给了完整的测试用例 可以不用自己手动写
    if (run_test_flag) {
        instr_test(myRiscv);    // 会自己去调用 test_riscv_instr()
    }

    // 根据模式定义判断是否以 debug 模式启动
    if (debug_mode) {
        gdb_server_t* gdb_server = gdb_server_create(myRiscv, default_debug_port, print_debug_info);

        // 打印成功提示信息
        printf("gdb is now running on port: %d\n", default_debug_port);
        myRiscv->gdb_server = gdb_server;
    }

    riscv_run(myRiscv);

    return 0;
}
