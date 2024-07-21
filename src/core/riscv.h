#ifndef RISCV_H
#define RISCV_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "types.h"
#include "instr.h"
#include "gdb/gdb_server.h"
#include "device/mem.h"

#define RISCV_REGS_SIZE                 32              // 寄存器数量

#define CSR_MSCRATCH        0x340

typedef struct _csr_regs_t {
    riscv_word_t mscratch;
}csr_regs_t;

typedef struct _breakpoint_t {
    riscv_word_t addr;
    struct _breakpoint_t * next;
}breakpoint_t;

/**
 * RISC-V
 */
typedef struct _riscv_t {
    gdb_server_t * server;          // gdb服务器

    device_t * dev_list;            // 设备列表
    device_t * dev_read;            // 当前操作的读设备
    device_t * dev_write;           // 当前操作的写设备
    mem_t * flash;                  // Flash空间

    riscv_word_t regs[RISCV_REGS_SIZE];
    riscv_word_t  pc;               // PC寄存器
    csr_regs_t csr_regs;            // CSR寄存器

    instr_t instr;                  // 当前解析的指令
    breakpoint_t * breaklist;       // 断点列表
} riscv_t;

#define riscv_read_reg(riscv, reg)  (riscv->regs[reg])
#define riscv_write_reg(riscv, reg, val) if ((reg != 0) && (reg < 32)) {riscv->regs[reg] = val;} 

riscv_t * riscv_create (void);
void riscv_add_device (riscv_t * riscv, device_t * dev);
void riscv_set_flash (riscv_t * riscv, mem_t * dev);
void riscv_reset (riscv_t * riscv);
void riscv_continue(riscv_t * riscv, int forever);
void riscv_run (riscv_t * riscv);

void riscv_load_bin (riscv_t * riscv, const char * file_name);
int riscv_mem_write(riscv_t * riscv, riscv_word_t addr, uint8_t * val, int width);
int riscv_mem_read(riscv_t * riscv, riscv_word_t addr, uint8_t * val, int width);

int riscv_add_breakpoint (riscv_t * riscv, riscv_word_t addr);
void riscv_remove_breakpoint (riscv_t * riscv, riscv_word_t addr);

#endif // !RISCV_H
