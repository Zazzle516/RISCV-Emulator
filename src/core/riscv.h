#ifndef RISCV_H
#define RISCV_H

#include "device/mem.h"
#include "instr.h"
#include "gdb/gdb_server.h"

#define RISCV_REG_NUM 32

// CSR 内存映射地址
#define RISCV_MSCRATCH  0x340

// 由于无法解决头文件的嵌套问题 所以还是写在同一个文件里
typedef struct _riscv_csr_t
{
    riscv_word_t mscratch;
}riscv_csr_t;


typedef struct _riscv_t
{
    riscv_word_t regs[RISCV_REG_NUM];       // 寄存器数组
    riscv_word_t pc;

    // 挂载 Flash 外设  其实就是添加该外设对应的结构体
    // 仍然需要对应的函数进行初始化
    mem_t* riscv_flash;

    // 对应 CPU 中 IR: Instruction Register
    instr_t instr;

    // 外部设备链表
    riscv_device_t* device_list;

    // 添加读写设备缓存
    riscv_device_t* dev_read_buffer;
    riscv_device_t* dev_write_buffer;

    // 定义 CSR 寄存器
    riscv_csr_t riscv_csr_regs;

    // 定义使用的 gdb 对象
    gdb_server_t* gdb_server;
    
}riscv_t;

// CSR 相关函数
void riscv_csr_init(riscv_t* riscv);
riscv_word_t riscv_read_csr(riscv_t* riscv, riscv_word_t addr);
void riscv_write_csr(riscv_t* riscv, riscv_word_t addr, riscv_word_t val);

/* 空间分配并且初始化 */
riscv_t* riscv_create(void);

/* 创建 Flash 外设对应的结构体 */

// Q: 为什么单独为 Flash 写一个函数
// A: 因为后续要根据参数动态分配空间
void riscv_flash_set(riscv_t* riscv, mem_t* flash);     // 注意没有返回值并且需要声明该 flash 需要挂载到哪个模拟器对象上

// 向该 flash->mem 空间中写入 image.bin 文件
void riscv_load_bin(riscv_t* riscv, const char* file_name);


/* 定义针对 Reg 的读写操作 注意读写操作一定要写在 riscv_t 结构体的声明后面 否则编译器无法识别 */

// 读操作: 根据模拟器对象 + reg 编号    这里用内联函数去写
// 更推荐内联函数的方式 因为宏函数并不会对参数进行判断
inline riscv_word_t riscv_read_reg(riscv_t* riscv, riscv_word_t reg) {
    return (riscv->regs[reg]);
}

// 写操作: 在模拟器层面 任何值都是二进制表示的 所以高级类型没有意义
// 注意因为 RISCV 的 ISA 在 reg(0) 写入是没有意义的 所以进行一个判断
#define riscv_write_reg(riscv, reg, val) if ((reg != 0) && (reg < 32)) {riscv->regs[reg] = val;}

// 在写入 image.bin 文件后对芯片进行重置
void riscv_reset(riscv_t* riscv);

// 模拟器核心执行流程
void riscv_continue(riscv_t* riscv, int step);

// 对外部设备读写
int riscv_mem_read(riscv_t* riscv, riscv_word_t start_addr, uint8_t* val, int width);
int riscv_mem_write(riscv_t* riscv, riscv_word_t start_addr, uint8_t* val, int width);

// 增加对不同存储设备的添加支持
void riscv_device_add(riscv_t* riscv, riscv_device_t* dev);

void riscv_run(riscv_t* riscv);

#endif /* RISCV_H */