#ifndef INSTR_IMPL_H
#define INSTR_IMPL_H

#include "riscv.h"
#include "types.h"
#include <stdio.h>

// 立即数统一用有符号数的方式处理

#define i_get_imm(instr)       \
    ((int32_t)(instr.i.imm11_0 | ((instr.i.imm11_0 & (1 << 11)) ? (0xFFFFF << 12) : 0)))

// I-Instruction-Math
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

// R-Instruction-Math
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

    riscv->pc += sizeof(riscv_word_t);
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
    riscv_word_t source1 = riscv_read_reg(riscv, riscv->instr.r.rs1);
    riscv_word_t source2 = riscv_read_reg(riscv, riscv->instr.r.rs2);

    riscv_word_t result = source1 & source2;
    riscv_write_reg(riscv, riscv->instr.r.rd, result);

    riscv->pc += sizeof(riscv_word_t);
}

// U-Instruction
static inline void handle_lui(riscv_t* riscv) {
    // 取出前 20 位放在寄存器中     地址是无符号数
    riscv_word_t imm20_0 = (riscv_word_t) riscv->instr.u.imm31_12 << 12;
    riscv_write_reg(riscv, riscv->instr.u.rd, imm20_0);
    riscv->pc += sizeof(riscv_word_t);
}

// 辅助函数: 获取一个 32 位有符号数
static inline int32_t s_get_offset(instr_t instr) {
    riscv_word_t temp = instr.s.imm11_5 << 5 | instr.s.imm4_0;       // 一共 12 位 需要转换到 32 位
    if (instr.s.imm11_5 & (1 << 6)) {
        // 负数 首位补 1
        return (int32_t)(temp | (0xFFFFF << 12));
    }
    return (int32_t)temp;
}

// S-Instruction-STORE
static inline void handle_sb(riscv_t* riscv) {
    // 将寄存器数据写到内存中
    riscv_word_t base_addr = riscv_read_reg(riscv, riscv->instr.s.rs1);
    int32_t offset = s_get_offset(riscv->instr);        // 注意 offset 是有符号数
    riscv_word_t target = riscv_read_reg(riscv, riscv->instr.s.rs2);
    
    // Q: 为什么在传具体值 target 的时候要传地址
    // A: 后续要通过地址处理 根据读写单位 uint8_t 和 width 决定写入多少
    riscv_mem_write(riscv, base_addr + offset, (uint8_t*) &target, 1);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_sh(riscv_t* riscv) {
    riscv_word_t base_addr = riscv_read_reg(riscv, riscv->instr.s.rs1);
    int32_t offset = s_get_offset(riscv->instr);
    riscv_word_t target = riscv_read_reg(riscv, riscv->instr.s.rs2);
    
    riscv_mem_write(riscv, base_addr + offset, (uint8_t*) &target, 2);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_sw(riscv_t* riscv) {
    riscv_word_t base_addr = riscv_read_reg(riscv, riscv->instr.s.rs1);
    int32_t offset = s_get_offset(riscv->instr);
    riscv_word_t target = riscv_read_reg(riscv, riscv->instr.s.rs2);

    riscv_mem_write(riscv, base_addr + offset, (uint8_t*) &target, 4);

    riscv->pc += sizeof(riscv_word_t);
}

// I-Instruction-LOAD
static inline void handle_lb(riscv_t* riscv) {
    // 从内存中加载数据到寄存器中
    riscv_word_t base_addr = riscv_read_reg(riscv, riscv->instr.i.rs1);
    int32_t offset = i_get_imm(riscv->instr);
    riscv_word_t load_addr = base_addr + offset;    // 获取要读取的数据的内存地址

    uint8_t res = 0;
    riscv_mem_read(riscv, load_addr, &res, 1);

    // 注意 res 是 uin8_t 类型 不能接收符号扩展后的结果 没有意义
    riscv_word_t new_val = res & (1 << 7) ? (res | 0xFFFFFF00) : res;
    riscv_write_reg(riscv, riscv->instr.i.rd, new_val);
    
    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_lh(riscv_t* riscv) {
    riscv_word_t base_addr = riscv_read_reg(riscv, riscv->instr.i.rs1);
    int32_t offset = i_get_imm(riscv->instr);
    riscv_word_t load_addr = base_addr + offset;

    uint16_t res = 0;
    riscv_mem_read(riscv, load_addr, (uint8_t*)&res, 2);
    riscv_word_t new_val = res & (1 << 15) ? (res | 0xFFFF0000) : res;
    riscv_write_reg(riscv, riscv->instr.i.rd, new_val);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_lw(riscv_t* riscv) {
    riscv_word_t base_addr = riscv_read_reg(riscv, riscv->instr.i.rs1);
    int32_t offset = i_get_imm(riscv->instr);
    riscv_word_t load_addr = base_addr + offset;

    uint32_t res = 0;
    riscv_mem_read(riscv, load_addr, (uint8_t*)&res, 4);    // 已经读满 不需要符号扩展
    riscv_write_reg(riscv, riscv->instr.i.rd, res);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_lbu(riscv_t* riscv) {
    // 不需要符号扩展的加载方式
    riscv_word_t base_addr = riscv_read_reg(riscv, riscv->instr.i.rs1);
    int32_t offset = i_get_imm(riscv->instr);
    riscv_word_t load_addr = base_addr + offset;

    uint8_t res = 0;
    riscv_mem_read(riscv, load_addr, &res, 1);
    riscv_write_reg(riscv, riscv->instr.i.rd, res);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_lhu(riscv_t* riscv) {
    // 不需要符号扩展的加载方式
    riscv_word_t base_addr = riscv_read_reg(riscv, riscv->instr.i.rs1);
    int32_t offset = i_get_imm(riscv->instr);
    riscv_word_t load_addr = base_addr + offset;

    uint16_t res = 0;
    riscv_mem_read(riscv, load_addr, (uint8_t*)&res, 2);
    riscv_write_reg(riscv, riscv->instr.i.rd, res);

    riscv->pc += sizeof(riscv_word_t);
}

// J-Instruction-Math
static inline void handle_auipc(riscv_t* riscv) {
    // 将立即数左移 12 位
    riscv_word_t imm = riscv->instr.u.imm31_12 << 12;
    riscv_word_t current_pc = riscv->pc;

    riscv_word_t new_addr = current_pc + imm;
    riscv_write_reg(riscv, riscv->instr.u.rd, new_addr);

    riscv->pc += sizeof(riscv_word_t);
}

// 辅助函数: 获取一个 32 位有符号数
static inline int32_t j_get_imm(instr_t instr) {
    riscv_word_t temp = (instr.j.imm_20 << 20) | (instr.j.imm_19_12 << 12) | (instr.j.imm_10_1 << 1) | (instr.j.imm_11 << 11);
    if (instr.j.imm_20 == 1) {
        // 负数补 1
        return (temp | 0xFFF00000);
    }
    return temp;
}

// J-Instruction-JUMP
static inline void handle_jal(riscv_t* riscv) {
    // 将立即数的结果存储在 pc 中
    int32_t offset = j_get_imm(riscv->instr);
    riscv_write_reg(riscv, riscv->instr.j.rd, riscv->pc + 4);
    riscv->pc = (riscv_word_t)(riscv->pc + offset);
    return;
}

// I-Instruction-JUMP
static inline void handle_jalr(riscv_t* riscv) {
    // 保存断点
    riscv_write_reg(riscv, riscv->instr.i.rd, riscv->pc + 4);

    // 跳转基地址
    riscv_word_t base = riscv_read_reg(riscv, riscv->instr.i.rs1);

    // 跳转偏移量
    int32_t offset = i_get_imm(riscv->instr);

    riscv->pc = base + offset;
    return;
}

// 辅助函数 
static inline int32_t b_get_imm(instr_t instr) {
    riscv_word_t temp = (instr.b.imm_4_1 << 1) | (instr.b.imm_10_5 << 5) | (instr.b.imm_11 << 11) | (instr.b.imm_12 << 12);
    if (instr.b.imm_12 == 1) {
        // 负数补 1
        return (int32_t)(temp | 0xFFFFE000);
    }
    return (int32_t)temp;
}

// B-Instrction-JUMP
static inline void handle_beq(riscv_t* riscv) {
    // 如果比较的两个寄存器的值相等 则进行跳转
    riscv_word_t source1 = riscv_read_reg(riscv, riscv->instr.b.rs1);
    riscv_word_t source2 = riscv_read_reg(riscv, riscv->instr.b.rs2);

    if (source1 == source2) {
        int32_t offset = b_get_imm(riscv->instr);
        riscv->pc = (offset + riscv->pc);
    } else {
        riscv->pc += sizeof(riscv_word_t);
    }
}

static inline void handle_bne(riscv_t* riscv) {
    // 如果比较的两个寄存器的值相等 则进行跳转
    riscv_word_t source1 = riscv_read_reg(riscv, riscv->instr.b.rs1);
    riscv_word_t source2 = riscv_read_reg(riscv, riscv->instr.b.rs2);

    if (source1 != source2) {
        int32_t offset = b_get_imm(riscv->instr);
        riscv->pc = (offset + riscv->pc);
    } else {
        riscv->pc += sizeof(riscv_word_t);
    }
}

static inline void handle_blt(riscv_t* riscv) {
    int32_t source1 = (int32_t)riscv_read_reg(riscv, riscv->instr.b.rs1);
    int32_t source2 = (int32_t)riscv_read_reg(riscv, riscv->instr.b.rs2);
    
    if (source1 < source2) {
        int32_t offset = b_get_imm(riscv->instr);
        riscv->pc = riscv->pc + offset;
    } else {
        riscv->pc += sizeof(riscv_word_t);
    }
}

static inline void handle_bge(riscv_t* riscv) {
    int32_t source1 = (int32_t)riscv_read_reg(riscv, riscv->instr.b.rs1);
    int32_t source2 = (int32_t)riscv_read_reg(riscv, riscv->instr.b.rs2);
    
    if (source1 >= source2) {
        int32_t offset = b_get_imm(riscv->instr);
        riscv->pc = riscv->pc + offset;
    } else {
        riscv->pc += sizeof(riscv_word_t);
    }
}

static inline void handle_bltu(riscv_t* riscv) {
    // 无符号比较 涉及到大小比较才会有符号问题  如果只是单纯的判断相等则不需要
    riscv_word_t source1 = riscv_read_reg(riscv, riscv->instr.b.rs1);
    riscv_word_t source2 = riscv_read_reg(riscv, riscv->instr.b.rs2);
    
    if (source1 < source2) {
        int32_t offset = b_get_imm(riscv->instr);
        riscv->pc = riscv->pc + offset;
    } else {
        riscv->pc += sizeof(riscv_word_t);
    }
}

static inline void handle_bgeu(riscv_t* riscv) {
    riscv_word_t source1 = riscv_read_reg(riscv, riscv->instr.b.rs1);
    riscv_word_t source2 = riscv_read_reg(riscv, riscv->instr.b.rs2);
    
    if (source1 >= source2) {
        int32_t offset = b_get_imm(riscv->instr);
        riscv->pc = riscv->pc + offset;
    } else {
        riscv->pc += sizeof(riscv_word_t);
    }
}

// M-Extension
static inline void handle_mul(riscv_t* riscv) {
    // 对于超出 32 位的乘法结果直接截断
    int32_t source1 = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs1);
    int32_t source2 = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs2);

    int32_t result = (int32_t)(source1 * source2);
    riscv_write_reg(riscv, riscv->instr.r.rd, result);

    riscv->pc += sizeof(riscv_word_t);
}

// 辅助函数 针对 64 位的扩展函数
static inline int64_t s_extend_64(int32_t temp) {
    if (temp < 0) {
        return (int64_t) (temp | (0xFFFFFFFF00000000));
    }
    return (int64_t)temp;
}

static inline void handle_mulh(riscv_t* riscv) {
    int64_t source1 = s_extend_64((int32_t) riscv_read_reg(riscv, riscv->instr.r.rs1));
    int64_t source2 = s_extend_64((int32_t) riscv_read_reg(riscv, riscv->instr.r.rs2));

    riscv_word_t result = ((source1 * source2) >> 32);
    riscv_write_reg(riscv, riscv->instr.r.rd, result);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_mulhsu(riscv_t* riscv) {
    // 执行一个有符号和无符号的乘法操作 并将高位放入目标寄存器中
    int64_t sign_source = s_extend_64((int32_t)riscv_read_reg(riscv, riscv->instr.r.rs1));
    uint64_t unsign_source = (uint64_t)riscv_read_reg(riscv, riscv->instr.r.rs2);

    riscv_word_t result = (sign_source * unsign_source) >> 32;
    riscv_write_reg(riscv, riscv->instr.r.rd, result);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_mulhu(riscv_t* riscv) {
    // 执行两个无符号的乘法操作 并将高位放入目标寄存器中
    uint64_t sign_source = (int64_t)riscv_read_reg(riscv, riscv->instr.r.rs1);
    uint64_t unsign_source = (uint64_t)riscv_read_reg(riscv, riscv->instr.r.rs2);

    riscv_word_t result = (sign_source * unsign_source) >> 32;
    riscv_write_reg(riscv, riscv->instr.r.rd, result);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_div(riscv_t* riscv) {
    int32_t rs1 = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs1);
    int32_t rs2 = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs2);

    riscv_write_reg(riscv, riscv->instr.r.rd, rs1 / rs2);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_divu(riscv_t* riscv) {
    riscv_word_t rs1 = riscv_read_reg(riscv, riscv->instr.r.rs1);
    riscv_word_t rs2 = riscv_read_reg(riscv, riscv->instr.r.rs2);

    riscv_write_reg(riscv, riscv->instr.r.rd, rs1 / rs2);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_rem(riscv_t* riscv) {
    int32_t rs1 = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs1);
    int32_t rs2 = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs2);

    riscv_write_reg(riscv, riscv->instr.r.rd, rs1 % rs2);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_remu(riscv_t* riscv) {
    riscv_word_t rs1 = riscv_read_reg(riscv, riscv->instr.r.rs1);
    riscv_word_t rs2 = riscv_read_reg(riscv, riscv->instr.r.rs2);

    riscv_write_reg(riscv, riscv->instr.r.rd, rs1 % rs2);

    riscv->pc += sizeof(riscv_word_t);
}

// CSR Operation
static inline void handle_csrrw(riscv_t* riscv) {
    // 读取 csr 到 reg-rd 中    同时把 reg-rs1 的值写入 csr 中
    riscv_word_t csr_addr = riscv->instr.i.imm11_0;

    riscv_word_t csr_content = riscv_read_csr(riscv, csr_addr);
    riscv_word_t rs1_content = riscv_read_reg(riscv, riscv->instr.i.rs1);

    riscv_write_reg(riscv, riscv->instr.i.rd, csr_content);
    riscv_write_csr(riscv, csr_addr, rs1_content);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_csrrs(riscv_t* riscv) {
    // 读取 csr 到 reg-rd 中    并根据 reg-rs1 的非零值将特定位的 csr 置 1
    riscv_word_t csr_addr = riscv->instr.i.imm11_0;
    
    riscv_word_t csr_content = riscv_read_csr(riscv, csr_addr);
    riscv_word_t rs1_content = riscv_read_reg(riscv, riscv->instr.i.rs1);

    riscv_write_csr(riscv, csr_addr, csr_content | rs1_content);
    riscv_write_reg(riscv, riscv->instr.i.rd, csr_content);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_csrrc(riscv_t* riscv) {
    // 读取 csr 到 reg-rd 中    并根据 reg-rs1 的非零值将特定位的 csr 置 0
    riscv_word_t csr_addr = riscv->instr.i.imm11_0;
    
    riscv_word_t csr_content = riscv_read_csr(riscv, csr_addr);
    riscv_word_t rs1_content = riscv_read_reg(riscv, riscv->instr.i.rs1);

    riscv_write_csr(riscv, csr_addr, csr_content & ~rs1_content);
    riscv_write_reg(riscv, riscv->instr.i.rd, csr_content);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_csrrwi(riscv_t* riscv) {
    // 先将 csr 写入 reg-rd    将立即数写入 csr
    riscv_word_t csr_addr = riscv->instr.i.imm11_0;
    riscv_word_t csr_content = riscv_read_csr(riscv, csr_addr);

    riscv_word_t new_csr_content = riscv->instr.i.rs1;

    riscv_write_reg(riscv, riscv->instr.i.rd, csr_content);
    riscv_write_csr(riscv, csr_addr, new_csr_content);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_csrrsi(riscv_t* riscv) {
    riscv_word_t csr_addr = riscv->instr.i.imm11_0;
    riscv_word_t csr_content = riscv_read_csr(riscv, csr_addr);

    riscv_write_reg(riscv, riscv->instr.i.rd, csr_content);

    riscv_word_t new_csr_content = riscv->instr.i.rs1;
    riscv_write_csr(riscv, csr_addr, new_csr_content | csr_content);

    riscv->pc += sizeof(riscv_word_t);
}

static inline void handle_csrrci(riscv_t* riscv) {
    riscv_word_t csr_addr = riscv->instr.i.imm11_0;

    riscv_word_t csr_content = riscv_read_csr(riscv, csr_addr);
    riscv_word_t new_csr_content = riscv->instr.i.rs1;

    riscv_write_reg(riscv, riscv->instr.i.rd, csr_content);
    riscv_write_csr(riscv, csr_addr, csr_content & ~new_csr_content);

    riscv->pc += sizeof(riscv_word_t);
}
#endif /* INSTER_IMPL_H */