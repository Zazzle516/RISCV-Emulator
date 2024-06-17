#include <string.h>
#include "plat/plat.h"
// #include <stdio.h>

// 注意，如果包含了instr_test.h，就不要再包含test.h，要去掉，不然可能有冲突
// #include "test.h"

// 模拟器的概念: 运行的是一个大型程序 只是模拟了 RISCV 的行为 所有调用的相关变量和物理资源都是我自己的电脑

#include "core/riscv.h"
#include "device/mem.h"
#include "test/instr_test.h"

#define RISCV_FLASH_START               0                       // 现在很多芯片地址都是从零开始
#define RISCV_FLASH_SIZE                (16 * 1024 * 1024)

#define RISCV_RAM_START                 0x20000000              // 根据测试工程决定在这个位置
#define RISCV_RAM_SIZE                  (16 * 1024 * 1024)

int main(int argc, char** argv) {
    plat_init();

    // 注意，去掉test_env后，注意要把test.h也给去掉
    // test_env();

    riscv_t* myRiscv = riscv_create();

    // 创建 RAM 空间
    // 通过 或操作 完成读写操作的同时定义
    mem_t* myRAM = mem_create("ram", RISCV_MEM_ATTR_READABLE | RISCV_MEM_ATTR_WRITABLE, RISCV_RAM_START, RISCV_RAM_SIZE);
    // 不要求外部设备类型一定是 Memory 所以是通过 device_t 结构体去控制  要进行强制类型转换
    // (riscv_device_t* myRAM)  也可以  只是挂了一个控制体上去  省区了 uint8_t* 的指针
    riscv_device_add(myRiscv, &myRAM->riscv_dev);

    // 创建 Flash 空间
    mem_t* myMemory = mem_create("flash", RISCV_MEM_ATTR_READABLE, RISCV_FLASH_START, RISCV_FLASH_SIZE);

    // 挂载到模拟器中
    riscv_device_add(myRiscv, (riscv_device_t*) myMemory);
    riscv_flash_set(myRiscv, myMemory);

    // 读取 image.bin 文件到 Flash 空间
    // flash_load_bin(myRiscv, "./unit/ebreak/obj/image.bin");
    // riscv_reset(myRiscv);
    // fprintf(stdout, "myRiscv has reset with pc = %d", myRiscv->pc);

    // 代码框架实际给了完整的测试用例 可以不用自己手动写
    instr_test(myRiscv);    // 会自己去调用 test_riscv_instr()

    return 0;
}
