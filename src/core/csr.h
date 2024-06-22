#ifndef CSR_REG_H
#define CSR_REG_H

#include "types.h"

#define CSR_MSCRATCH        0x340

typedef struct _csr_regs_t {
    riscv_word_t mscratch;
}csr_reg_t;

void riscv_csr_init(csr_reg_t riscv_csr_reg);

#endif /* CSR_REG_H */