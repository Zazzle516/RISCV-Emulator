#ifndef DEVICCE_H
#define DEVICE_H
#include "core/types.h"

// device.h: 声明一个统一的外设数据结构 + 初始化方式(所以并不包含对空间的分配, 具体的空间应该由具体的外设决定)
            // 否则初始化函数会命名为 create()

typedef struct _riscv_device_t
{
    const char* name;   // 可以常量的用 const 修饰
    riscv_word_t attr;
    riscv_word_t addr_start;
    riscv_word_t addr_end;
}riscv_device_t;

// sizeof = char*(4B) + ref(4B * 3) = 16B

// 在 C 语言中没有高级语言之类的构造函数 所以要主动传结构体(类)指针进去
void device_init(riscv_device_t* myDev, const char* name, riscv_word_t attr,
                   riscv_word_t start, riscv_word_t size);

#endif /* DEVICE_H */