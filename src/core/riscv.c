#include "riscv.h"
#include<stdlib.h>
#include<assert.h>

riscv_t* riscv_create(void) {
    riscv_t* riscv = (riscv_t*)calloc(1, sizeof(riscv_t));    // 因为要求返回指针 所以分配一个空间就可以    32 + 32 + 32*32 / 144
    assert(riscv != NULL);  // 判断为 True 继续运行
    return riscv;
}

void riscv_flash_set(riscv_t* riscv, mem_t* flash) {
    riscv->riscv_flash = flash;
}

