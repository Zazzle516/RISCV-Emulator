#include "device.h"
#include <string.h>

/**
 * 初始化RISC-V设备对像
*/
void device_init(device_t * dev, const char * name, riscv_word_t attr, riscv_word_t base, riscv_word_t size) {
    memset(dev, 0, sizeof(device_t));
    dev->name = name;
    dev->attr = attr;
    dev->base = base;
    dev->end = base + size;
}