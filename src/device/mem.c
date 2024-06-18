#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mem.h"

// 仅使用在本文件的 static 的关键字要全部使用 static 修饰
// 注意 mem_read() 和 mem_write() 函数一定要写在 mem_create() 上面!!!!  否则里面的定义找不到 nmd

// 针对 mem 存储器的读写函数    从 addr 读取到 val 中
static int mem_read (struct _riscv_device_t* dev, riscv_word_t addr, uint8_t* val, int width) {    
    // 检查读写权限
    if ((dev->attr & RISCV_MEM_ATTR_READABLE) == 0) {
        // 没有读取权限
        fprintf(stderr, "memory read failed at %x\n", addr);
        return -1;
    }
    
    // Q: 为什么要扩大类型
    // A: 因为要访问到 mem_t 结构体中指向内存空间的指针
    mem_t* myMemory = (mem_t*) dev;     // 扩大的强制类型转换   需要在后续传参的时候注意    Q: 以及为什么已经决定扩大了还要传 riscv_device_t 类型
    
    // Q: 为什么要用偏移量
    // A: 模拟器的物理地址和本机物理地址的区别  最终的地址是一个 64 位的地址
    
    // Q: 为什么偏移量用 uint32_t 接收  和 int32_t 的偏移量有什么区别
    // A: 因为是逻辑地址并且一定高于起始地址 所以结果一定是正数
    riscv_word_t offset = addr - dev->addr_start;

    if (width == 4) {
        *(uint32_t*) val = *(uint32_t*) (myMemory->mem + offset);
    } else if (width == 2) {
        *(uint16_t*) val = *(uint16_t*) (myMemory->mem + offset);
    } else if (width == 1) {
        *(uint8_t*) val = *(uint8_t*) (myMemory->mem + offset);
    } else {
        memcpy(val, myMemory->mem + offset, width);
    }
    return 0;
}

// 把 val 的内容写到 addr 中
static int mem_write (struct _riscv_device_t * dev,  riscv_word_t addr, uint8_t * val, int size) {
    if ((dev->attr & RISCV_MEM_ATTR_WRITABLE) == 0) {
        fprintf(stderr, "write to write-only memory\n");
        return -1;
    }

    mem_t * mem = (mem_t *)dev;
    riscv_word_t offset = addr - dev->addr_start;
    if (size == 4) {
        *(riscv_word_t *)(mem->mem + offset) = *(riscv_word_t *)val;
    } else if (size == 2) {
        *(uint16_t *)(mem->mem + offset) = *(uint16_t *)val;
    } else if (size == 1) {
        *(uint8_t *)(mem->mem + offset) = *(uint8_t *)val;
    } else {
        memcpy(mem->mem + offset, val, size);
    }

    return 0;
}

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

    riscv_device_t* dev = (riscv_device_t*) myFlash;    // Q: 可不可以认为是从 myFlash 中切割了一块空间出来 通过 flashDevice 指向
    
    // riscv_device_t* flashDev = &myFlash->riscv_dev;

    device_init(dev, name, attr, start, size);

    // 初始化 dev 对应的读写操作
    dev->read = mem_read;
    dev->write = mem_write;
    return myFlash;
}

