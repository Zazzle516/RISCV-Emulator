#ifndef MEMORY_H
#define MEMORY_H

#include "device.h"     // 某种程度上可以看成是继承自父类 device

// 在具体外设中进行实际的空间分配 所以最后一定要记得 free() 掉

// 定义读写权限 因为 Flash 和 RAM 权限不同 （Flash 属于 ROM 只读存储器
#define RISCV_MEM_ATTR_READABLE     (1 << 0)
#define RISCV_MEM_ATTR_WRITABLE     (1 << 1)

typedef struct _mem_t{
    riscv_device_t riscv_dev;    // 待分配的外设数据结构空间

    uint8_t* mem;                // 指向 Flash 空间的指针
}mem_t;

// 因为在 mem_t 结构体中传递的并不是 riscv_device_t 指针 所以这里应该传递所有参数
// mem_t* mem_create(riscv_device_t* riscv_dev, mem_t* flash);

// 对比 device_init() 这里需要返回一个指针
mem_t* mem_create(const char* name, riscv_word_t attr, riscv_word_t start, riscv_word_t size);

#endif /* MEMORY_H */