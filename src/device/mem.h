#ifndef MEMORY_H
#define MEMORY_H

#include "device.h"

#define RISCV_MEM_ATTR_READABLE     (1 << 0)
#define RISCV_MEM_ATTR_WRITABLE     (1 << 1)

typedef struct _riscv_mem_t {
    device_t device;            // 基础设备
    uint8_t * mem;              // 存储区域
}mem_t;

mem_t * mem_create(const char * name, riscv_word_t base, riscv_word_t size, riscv_word_t attr);

#endif 