#ifndef INSTR_H
#define INSTR_H
#include "types.h"
// 因为 instr_test.h 中调用了 riscv->instr 结构体

// 复制粘贴一下指令信息
#define OP_LUI     0b0110111
#define OP_AUIPC   0b0010111
#define OP_JAL     0b1101111
#define OP_JALR    0b1100111
#define OP_BEQ     0b1100011
#define OP_BNE     0b1100011
#define OP_BLT     0b1100011
#define OP_BGE     0b1100011
#define OP_BLTU    0b1100011
#define OP_BGEU    0b1100011
#define OP_LB      0b0000011
#define OP_LH      0b0000011
#define OP_LW      0b0000011
#define OP_LBU     0b0000011
#define OP_LHU     0b0000011
#define OP_SB      0b0100011
#define OP_SH      0b0100011
#define OP_SW      0b0100011
#define OP_ADDI    0b0010011
#define OP_SLTI    0b0010011
#define OP_SLTIU   0b0010011
#define OP_XORI    0b0010011
#define OP_ORI     0b0010011
#define OP_ANDI    0b0010011
#define OP_SLLI    0b0010011
#define OP_SRLI    0b0010011
#define OP_SRAI    0b0010011
#define OP_ADD     0b0110011
#define OP_SUB     0b0110011
#define OP_SLL     0b0110011
#define OP_SLT     0b0110011
#define OP_SLTU    0b0110011
#define OP_XOR     0b0110011
#define OP_SRL     0b0110011
#define OP_SRA     0b0110011
#define OP_OR      0b0110011
#define OP_AND     0b0110011
#define OP_NOP     0b0010011
#define OP_CSR     0b1110011

#define FUNC3_ADDI      0b000
#define FUNC3_SLTI      0b010
#define FUNC3_SLTIU     0b011
#define FUNC3_XORI      0b100
#define FUNC3_ORI       0b110
#define FUNC3_ANDI      0b111
#define FUNC3_SLLI      0b001
#define FUNC3_SR        0b101

#define FUNC3_SB        0b000
#define FUNC3_SH        0b001
#define FUNC3_SW        0b010

#define FUNC3_LB        0b000
#define FUNC3_LH        0b001
#define FUNC3_LW        0b010
#define FUNC3_LBU       0b100
#define FUNC3_LHU       0b101

#define FUNC3_ADD       0b000
#define FUNC3_SUB       0b000
#define FUNC3_SLL       0b001
#define FUNC3_SLT       0b010
#define FUNC3_SLTU      0b011
#define FUNC3_XOR       0b100
#define FUNC3_SRL       0b101
#define FUNC3_SRA       0b101
#define FUNC3_OR        0b110
#define FUNC3_AND       0b111

#define FUNC3_BEQ       0b000
#define FUNC3_BNE       0b001
#define FUNC3_BLT       0b100
#define FUNC3_BGE       0b101
#define FUNC3_BLTU      0b110
#define FUNC3_BGEU      0b111

#define FUNC3_MUL       0b000
#define FUNC3_MULH      0b001
#define FUNC3_MULHSU    0b010
#define FUNC3_MULHU     0b011
#define FUNC3_DIV       0b100
#define FUNC3_DIVU      0b101
#define FUNC3_REM       0b110
#define FUNC3_REMU      0b111

#define FUNC3_CSRRW     0b001
#define FUNC3_CSRRS     0b010
#define FUNC3_CSRRC     0b011
#define FUNC3_CSRRWI    0b101
#define FUNC3_CSRRSI    0b110
#define FUNC3_CSRRCI    0b111
#define FUNC3_RETURN    0b000

#define FUNC7_ADD       0b0000000
#define FUNC7_SUB       0b0100000
#define FUNC7_SRL       0b0000000
#define FUNC7_SRA       0b0100000
#define FUNC7_DIV       0b0000001
#define FUNC7_MUL       0b0000001
#define FUNC7_REM       0b0000001
#define FUNC7_MRET      0b0011000

#define EBREAK 0b00000000000100000000000001110011
#define OP_BREAK 0b1110011

typedef union _instr_t
{
    // common 单独的 OPCODE 特化分类
    struct {
        riscv_word_t opcode : 7;
        riscv_word_t other : 25;
    };

    // R-Type
    struct {
        riscv_word_t funct7 : 7;
        riscv_word_t rs2 : 5;
        riscv_word_t rs1 : 5;
        riscv_word_t funct3 : 3;
        riscv_word_t rd : 5;
        riscv_word_t opcode : 7;
    } r;

    // I-Type
    struct {
        // riscv_word_t imm11_0 : 12;
        // riscv_word_t rs1 : 5;
        // riscv_word_t funct3 : 3;
        // riscv_word_t rd : 5;
        // riscv_word_t opcode : 7;
        
        riscv_word_t opcode : 7;
        riscv_word_t rd : 5;
        riscv_word_t funct3 : 3;
        riscv_word_t rs1 : 5;
        riscv_word_t imm11_0 : 12;
    } i;

    // S-Type
    struct {
        riscv_word_t imm11_5 : 7;
        riscv_word_t rs2 : 5;
        riscv_word_t rs1 : 5;
        riscv_word_t funct3 : 3;
        riscv_word_t imm4_0 : 5;
        riscv_word_t opcode : 7;
    } s;

    // B-Type
    struct {
        riscv_word_t imm_12 : 1;
        riscv_word_t imm_10_5 : 6;
        riscv_word_t rs2 : 5;
        riscv_word_t rs1 : 5;
        riscv_word_t funct3 : 3;
        riscv_word_t imm_4_1 : 4;
        riscv_word_t imm_11 : 1;
        riscv_word_t opcode : 7;
    } b;

    // U-Type
    struct {
        riscv_word_t imm31_12 : 20;
        riscv_word_t rd : 5;
        riscv_word_t opcode : 7;
    } u;

    // J-Type
    struct {
        riscv_word_t imm_20 : 1;
        riscv_word_t imm_10_1 : 10;
        riscv_word_t imm_11 : 1;
        riscv_word_t imm_19_12 : 8;
        riscv_word_t rd : 5;
        riscv_word_t opcode : 7;
    } j;

    riscv_word_t raw;
}instr_t;

#endif /* INSTR_H */