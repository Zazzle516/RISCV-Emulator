#include "device.h"

void device_init(riscv_device_t* myDev, const char* name, riscv_word_t attr, riscv_word_t start, riscv_word_t size){
    // 这里因为所需要的字段碰巧都会被参数定义 所以省去了对这片空间的初始化步骤
    // 只是初始化空间并不是分配空间, 正常来说需要清零的
    myDev->name = name;
    myDev->attr = attr;
    myDev->addr_start = start;
    myDev->addr_end = start + size;
}