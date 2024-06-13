#include "riscv.h"
#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>

riscv_t* riscv_create(void) {
    riscv_t* riscv = (riscv_t*)calloc(1, sizeof(riscv_t));    // 因为要求返回指针 所以分配一个空间就可以    32 + 32 + 32*32 / 144
    assert(riscv != NULL);  // 判断为 True 继续运行
    return riscv;
}

// 挂载 flash 结构体
void riscv_flash_set(riscv_t* riscv, mem_t* flash) {
    riscv->riscv_flash = flash;
}

// 读取 image.bin 文件
void flash_load_bin(riscv_t* riscv, const char* file_name) {
    // 相对路径在 launch.json 文件定义的根路径中
    // 注意指定读取方式 以二进制 b 的形式读取
    FILE* file = fopen(file_name, "rb");

    // 判断文件是否存在
    if (file == NULL) {
        fprintf(stderr, "file %s doesn't exist\n", file_name);
        exit(-1);
    }

    // 判断文件是否为空 程序报错退出
    if (feof(file)) {
        fprintf(stderr, "file %s is empty\n", file_name);
        exit(-1);
    }

    // 确认文件成功打开后写入数据(有缓冲区的方式)
    char buffer[1024];
    char* destination = riscv->riscv_flash->mem;    // 以写入的数据大小作为类型
    
    size_t size;

    // 之前我这里的 size 赋值少加了括号 然后优先级判定 size = 1 出了 bug
    while ((size = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        // 判断大于 0 只是判断仍然有内容没有写入 所以不一定每次都是 1024
        memcpy(destination, buffer, size);
        destination += size;
    }

    // 记得关闭
    fclose(file);
}

// 重置芯片状态
void riscv_reset(riscv_t* riscv) {
    riscv->pc = 0;      // 因为 main.c 中定义的 RISCV_FLASH_START 是从 0 开始
    memset(riscv->regs, 0, sizeof(riscv->regs));

    // 并没有重置 Flash 和 RAM
}