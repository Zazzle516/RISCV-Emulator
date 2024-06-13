#ifndef INSTR_H
#define INSTR_H
#include "types.h"
// 因为 instr_test.h 中调用了 riscv->instr 结构体

#define EBREAK 0b00000000000100000000000001110011
#define OP_BREAK 0b1110011

typedef union instr
{
    struct {
        riscv_word_t opcode : 7;
        riscv_word_t other: 25;
    };
    riscv_word_t raw;
}instr_t;

#endif /* INSTR_H */