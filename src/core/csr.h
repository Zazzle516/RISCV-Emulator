#ifndef CSR_REG_H
#define CSR_REG_H

#include "types.h"

#define CSR_MSCRATCH        0x340

// 前向声明 riscv_t
struct riscv_t;

// 定义当前结构体
typedef struct _csr_regs_t {
    riscv_word_t mscratch;
}csr_reg_t;

void riscv_csr_init(riscv_t* riscv);

#endif /* CSR_REG_H */