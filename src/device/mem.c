#include <stdlib.h>
#include <stdio.h>

#include "mem.h"

mem_t* mem_create(const char* name, riscv_word_t attr, riscv_word_t start, riscv_word_t size) {
    // 1.分配 device 空间并且初始化     2.分配对应的内存空间
    mem_t* myFlash = (mem_t*)calloc(1, sizeof(mem_t));
    
    // 判断是否分配成功
    if (myFlash == NULL) {
        fprintf(stderr, "memory alloc faild\n");    // 注意初始化是为零不是为空
        return NULL;
    }

    // 分配真正的 Flash 空间
    myFlash->mem = malloc(size);    // 返回 void* 表示未确定类型的指针 如果分配成功返回对应空间的首地址

    // 同理判断
    if (myFlash->mem == NULL) {
        fprintf(stderr, "No enough space to malloc");
        free(myFlash);
        return NULL;   // 没有足够的空间可以分配 直接退出
    }

    riscv_device_t* flashDevice = (riscv_device_t*) myFlash;    // Q: 可不可以认为是从 myFlash 中切割了一块空间出来 通过 flashDevice 指向
    
    // riscv_device_t* flashDev = &myFlash->riscv_dev;

    device_init(flashDevice, name, attr, start, size);

    return myFlash;
}
