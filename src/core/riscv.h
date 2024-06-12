#ifndef RISCV_H
#define RISCV_H

#include "core/types.h"
#include "device/mem.h"

#define RISCV_REG_NUM 32


typedef struct _riscv_t
{
    riscv_word_t regs[RISCV_REG_NUM];       // 寄存器数组
    riscv_word_t pc;

    // 挂载 Flash 外设  其实就是添加该外设对应的结构体
    // 仍然需要对应的函数进行初始化
    mem_t* riscv_flash;
}riscv_t;

/* 空间分配并且初始化 */
riscv_t* riscv_create(void);

/* 创建 Flash 外设对应的结构体 */
// Q: 此时的这个 Flash 是什么状态 已经写入对应的 bin 文件了吗

// Q: 为什么单独为 Flash 写一个函数
// A: 因为后续要根据参数动态分配空间
void riscv_flash_set(riscv_t* riscv, mem_t* flash);     // 注意没有返回值并且需要声明该 flash 需要挂载到哪个模拟器对象上

// 向该 flash->mem 空间中写入 image.bin 文件
void flash_load_bin(riscv_t* riscv, const char* file_name);


/* 定义针对 Reg 的读写操作 注意读写操作一定要写在 riscv_t 结构体的声明后面 否则编译器无法识别 */

// 读操作: 根据模拟器对象 + reg 编号    这里用内联函数去写
inline riscv_word_t riscv_read_regs(riscv_t* riscv, riscv_word_t reg) {
    return (riscv->regs[reg]);
}

// 写操作: 在模拟器层面 任何值都是二进制表示的 所以高级类型没有意义
// 注意因为 RISCV 的 ISA 在 reg(0) 写入是没有意义的 所以进行一个判断
#define riscv_write_reg (riscv, reg, val) (if (reg != 0) riscv->regs[reg] = val; )

#endif /* RISCV_H */