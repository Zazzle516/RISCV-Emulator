#ifndef INSTR_IMPL_H
#define INSTR_IMPL_H

#include "riscv.h"
#include "types.h"
#include <stdio.h>

// 立即数统一用有符号数的方式处理

#define i_get_imm(instr)       \
    ((int32_t)(instr.i.imm11_0 | ((instr.i.imm11_0 & (1 << 11)) ? (0xFFFFF << 12) : 0)))

// I-Instruction
static inline void handle_addi(riscv_t* riscv) {
    int32_t source = (int32_t)riscv_read_reg(riscv, riscv->instr.i.rs1);
    int32_t imm = (int32_t)i_get_imm(riscv->instr);
    int32_t result = imm + source;
    riscv_write_reg(riscv, riscv->instr.i.rd, result);
                
    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_slti(riscv_t* riscv) {
    // 将寄存器的有符号整数与立即数比较
    int32_t source = (int32_t)riscv_read_reg(riscv, riscv->instr.i.rs1);
    int32_t imm = (int32_t)i_get_imm(riscv->instr);
    int32_t result = (source < imm) ? 1 : 0;
    riscv_write_reg(riscv, riscv->instr.i.rd, result);
                
    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_sltiu(riscv_t* riscv) {
    // 与 SLTI 指令相比要求无符号数
    riscv_word_t source = (riscv_word_t)riscv_read_reg(riscv, riscv->instr.i.rs1);
    riscv_word_t imm = i_get_imm(riscv->instr);     // Q: 为什么需要对 imm 进行有符号扩展
    riscv_word_t result = (source < imm) ? 1 : 0;
    riscv_write_reg(riscv, riscv->instr.i.rd, result);
                
    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_xori(riscv_t* riscv) {
    // 与立即数进行按位异或运算
    riscv_word_t source = (riscv_word_t)riscv_read_reg(riscv, riscv->instr.i.rs1);
    int32_t imm = i_get_imm(riscv->instr);
    riscv_word_t result = source ^ imm;
    riscv_write_reg(riscv, riscv->instr.i.rd, result);
                
    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_ori(riscv_t* riscv) {
    riscv_word_t source = riscv_read_reg(riscv, riscv->instr.i.rs1);
    int32_t imm = (int32_t)i_get_imm(riscv->instr);
    riscv_word_t result = imm | source;
    riscv_write_reg(riscv, riscv->instr.i.rd, result);
                
    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_andi(riscv_t* riscv) {
    // Q: 因为是逻辑运算 这里应该不考虑符号问题吧
    riscv_word_t source = (riscv_word_t)riscv_read_reg(riscv, riscv->instr.i.rs1);
    int32_t imm = i_get_imm(riscv->instr);
    riscv_word_t result = imm & source;
    riscv_write_reg(riscv, riscv->instr.i.rd, result);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_slli(riscv_t* riscv) {
    // 逻辑左移指定位数 逻辑右移统一补零
    riscv_word_t source = (riscv_word_t)riscv_read_reg(riscv, riscv->instr.i.rs1);
    riscv_word_t imm = (riscv_word_t)i_get_imm(riscv->instr) & 0b11111;
    
    riscv_word_t result = source << imm;
    riscv_write_reg(riscv, riscv->instr.i.rd, result);
                
    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_srai_srli(riscv_t* riscv) {
    riscv_word_t flag = riscv->instr.i.imm11_0 >> 5;
    if (flag > 0) {
        riscv_word_t source = (riscv_word_t)riscv_read_reg(riscv, riscv->instr.i.rs1);
        riscv_word_t shamt = riscv->instr.i.imm11_0 & 0b11111;   // 只取出最后 5 位
        
        // 算术移位需要判断 source 首位正负
        int32_t flag = source >> 31;
        if (flag == 0) {
            // 正数情况 等效于逻辑右移
            riscv_word_t result = (source >> shamt);
            riscv_write_reg(riscv, riscv->instr.i.rd, result);
        }
        else {
            // 负数情况 右移前面的部分补 1
            int32_t result1 = (0xFFFFFFFF << (32 - shamt)) | (source >> shamt);
            // 可以不用那么麻烦 直接转换为有符号数就可以了
            int32_t result2 = (int32_t) source >> shamt;
            if (result1 != result2) {
                printf("handler srai not ok on minus number\n");
            }
            riscv_write_reg(riscv, riscv->instr.i.rd, result2);
        }
    }

    else {
        riscv_word_t source = (riscv_word_t)riscv_read_reg(riscv, riscv->instr.i.rs1);
        riscv_word_t shamt = (riscv_word_t)i_get_imm(riscv->instr) & 0b11111;   // 只取出最后 5 位
        riscv_word_t result = source >> shamt;
        riscv_write_reg(riscv, riscv->instr.i.rd, result);
    }

    riscv->pc += sizeof(riscv_word_t);
}

// R-Instruction
static inline void handle_add(riscv_t* riscv) {
    int32_t source1 = (int32_t) riscv_read_reg(riscv, riscv->instr.r.rs1);
    int32_t source2 = (int32_t) riscv_read_reg(riscv, riscv->instr.r.rs2);
    int32_t result = source1 + source2;
    riscv_write_reg(riscv, riscv->instr.r.rd, result);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_sub(riscv_t* riscv) {
    int32_t source1 = (int32_t) riscv_read_reg(riscv, riscv->instr.r.rs1);
    int32_t source2 = (int32_t) riscv_read_reg(riscv, riscv->instr.r.rs2);
    int32_t result = source1 - source2;
    riscv_write_reg(riscv, riscv->instr.r.rd, result);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_sll(riscv_t* riscv) {
    // 将寄存器中的数值左移 在模拟器层面不需要考虑前后的符号一致性
    riscv_word_t source = (riscv_word_t) riscv_read_reg(riscv, riscv->instr.r.rs1);
    riscv_word_t shamt = (riscv_word_t) riscv_read_reg(riscv, riscv->instr.r.rs2);
    riscv_word_t result = source << shamt;
    riscv_write_reg(riscv, riscv->instr.r.rd, result);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_slt(riscv_t* riscv) {
    // 比较两个寄存器中有符号整数值 结果存储在 rd 中
    int32_t source1 = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs1);
    int32_t source2 = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs2);
    riscv_word_t flag = (source1 < source2) ? 1 : 0;
    riscv_write_reg(riscv, riscv->instr.r.rd, flag);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_sltu(riscv_t* riscv) {
    // 比较两个寄存器中无符号整数值 结果存储在 rd 中
    riscv_word_t source1 = (riscv_word_t)riscv_read_reg(riscv, riscv->instr.r.rs1);
    riscv_word_t source2 = (riscv_word_t)riscv_read_reg(riscv, riscv->instr.r.rs2);
    riscv_word_t flag = (source1 < source2) ? 1 : 0;
    riscv_write_reg(riscv, riscv->instr.r.rd, flag);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_xor(riscv_t* riscv) {
    // 对两个寄存器中的数按位异或
    riscv_word_t source1 = (riscv_word_t)riscv_read_reg(riscv, riscv->instr.r.rs1);
    riscv_word_t source2 = (riscv_word_t)riscv_read_reg(riscv, riscv->instr.r.rs2);
    riscv_word_t flag = source1 ^ source2;
    riscv_write_reg(riscv, riscv->instr.r.rd, flag);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_srl(riscv_t* riscv) {
    // 逻辑右移
    riscv_word_t source1 = (riscv_word_t) riscv_read_reg(riscv, riscv->instr.r.rs1);
    riscv_word_t source2 = (riscv_word_t) riscv_read_reg(riscv, riscv->instr.r.rs2);
    riscv_word_t shamt = source2 & 0b11111;

    riscv_word_t result = source1 >> shamt;
    riscv_write_reg(riscv, riscv->instr.r.rd, result);
}

static inline void handle_sra(riscv_t* riscv) {
    int32_t source1 = (int32_t) riscv_read_reg(riscv, riscv->instr.r.rs1);
    int32_t source2 = (int32_t) riscv_read_reg(riscv, riscv->instr.r.rs2);
    riscv_word_t shamt = source2 & 0b11111;     // ((uint32_t)source1 >> 9) | (0xFFFFFFFF << (32 - shamt))

    if (source1 < 0) {
        // 负数 右移补 1
        int32_t result1 = ((uint32_t)source1 >> shamt) | (0xFFFFFFFF << (32 - shamt));
        int32_t result2 = ((int32_t) source1) >> shamt;
        if (result1 != result2) {
            printf("handle sra on minus number not ok\n");
        }
        riscv_write_reg(riscv, riscv->instr.r.rd, result2);
    }
    else {
        // 正数 等效于逻辑右移
        riscv_word_t result = source1 >> shamt;
        riscv_write_reg(riscv, riscv->instr.r.rd, result);
    }
}

static inline void handle_srl_sra(riscv_t* riscv) {
    riscv_word_t flag = riscv->instr.r.funct7;
    if (flag > 0) {
        handle_sra(riscv);
    }
    else {
        handle_srl(riscv);
    }

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_or(riscv_t* riscv) {
    // 对两个寄存器中的数或运算
    riscv_word_t source1 = (riscv_word_t)riscv_read_reg(riscv, riscv->instr.r.rs1);
    riscv_word_t source2 = (riscv_word_t)riscv_read_reg(riscv, riscv->instr.r.rs2);
    riscv_word_t flag = source1 | source2;
    riscv_write_reg(riscv, riscv->instr.r.rd, flag);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_and(riscv_t* riscv) {
    
}

#endif /* INSTER_IMPL_H */