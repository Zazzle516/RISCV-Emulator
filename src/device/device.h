#ifndef RISCV_DEVICE_H
#define RISCV_DEVICE_H

#include "core/types.h"

struct _riscv_t;

/**
 * RISC-V外部设备
*/
typedef struct _riscv_device_t {
    struct _riscv_t * riscv;
    const char * name;
    riscv_word_t attr;
    riscv_word_t base;
    riscv_word_t end;
    struct _riscv_device_t * next;

    int (*read) (struct _riscv_device_t * dev, riscv_word_t addr, uint8_t * data, int width);
    int (*write) (struct _riscv_device_t * dev,  riscv_word_t addr, uint8_t * data, int width);
} device_t;

void device_init(device_t * dev, const char * name, riscv_word_t attr, riscv_word_t base, riscv_word_t size);


#endif