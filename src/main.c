#include <string.h>
#include "plat/plat.h"

// 注意，如果包含了instr_test.h，就不要再包含test.h，要去掉，不然可能有冲突
// #include "test.h"

// 模拟器的概念: 运行的是一个大型程序 只是模拟了 RISCV 的行为 所有调用的相关变量和物理资源都是我自己的电脑

#include "core/riscv.h"
#include "device/mem.h"

#define RISCV_FLASH_START               0                       // 现在很多芯片地址都是从零开始
#define RISCV_FLASH_SIZE                (16 * 1024 * 1024)

int main(int argc, char** argv) {
    plat_init();

    // 注意，去掉test_env后，注意要把test.h也给去掉
    // test_env();

    riscv_t* myRiscv = riscv_create();

    // 创建空间
    mem_t* myMemory = mem_create("flash", RISCV_MEM_ATTR_READABLE, RISCV_FLASH_START, RISCV_FLASH_SIZE);

    // 挂载到模拟器中
    riscv_flash_set(myRiscv, myMemory);

    // 读取 image.bin 文件到 Flash 空间
    flash_load_bin(myRiscv, "./unit/ebreak/obj/image.bin");

    return 0;
}
