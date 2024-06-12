#include <string.h>
#include "plat/plat.h"

// 注意，如果包含了instr_test.h，就不要再包含test.h，要去掉，不然可能有冲突
// #include "test.h"

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
    return 0;
}
