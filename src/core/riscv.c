#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plat/plat.h"
#include "instr.h"
#include "riscv.h"
#include "device/mem.h"


/**
 * 创建一个RISCV内核对像
*/
riscv_t* riscv_create(void) {
    // 创建RISC-V对像
    riscv_t* riscv = (riscv_t*)calloc(1, sizeof(riscv_t));
    assert(riscv != NULL);
    return riscv;
} 

/**
 * 添加一个外部设备
*/
void riscv_add_device (riscv_t * riscv, device_t * dev) {
    dev->next = riscv->dev_list;
    riscv->dev_list = dev;
    if (riscv->dev_read == NULL) {
        riscv->dev_read = dev;
    }
    if(riscv->dev_write == NULL) {
        riscv->dev_write = dev;
    }

    dev->riscv = riscv;
}

void riscv_set_flash (riscv_t * riscv, mem_t * dev) {
    riscv->flash = dev;
}

/**
 * 查找一个存储区
*/
device_t * riscv_find_device(riscv_t * riscv, riscv_word_t addr) {
    device_t * curr = riscv->dev_list;
    while (curr) {
        if ((addr >= curr->base) && (addr < curr->end)) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

int riscv_mem_write(riscv_t * riscv, riscv_word_t addr, uint8_t * val, int width) {
    if ((addr >= riscv->dev_write->base) && (addr < riscv->dev_write->end)) {
        return riscv->dev_write->write(riscv->dev_write, addr, val, width);
    }

    device_t * device = riscv_find_device(riscv, addr);
    if (device == NULL) {
        fprintf(stderr, "write to invalid address %x\n", addr);
        return -1;
    }

    riscv->dev_write = device;
    return device->write(device, addr,val, width);
}

int riscv_mem_read(riscv_t * riscv, riscv_word_t addr, uint8_t * val, int width) {
    if ((addr >= riscv->dev_read->base) && (addr < riscv->dev_read->end)) {
        return riscv->dev_read->read(riscv->dev_read, addr, val, width);
    }

    device_t * device = riscv_find_device(riscv, addr);
    if (device == NULL) {
        fprintf(stderr, "read from invalid address %x\n", addr);
        return -1;
    }

    riscv->dev_read = device;
    return device->read(device, addr, val, width);
}

void riscv_csr_init (riscv_t * riscv) {
}

/**
 * 写CSR寄存器
*/
void riscv_write_csr(riscv_t * riscv, riscv_word_t csr, riscv_word_t val) {
    riscv->csr_regs.mscratch = val;
}

/**
 * 读CSR寄存器
*/
riscv_word_t riscv_read_csr(riscv_t * riscv, riscv_word_t csr) {
    return riscv->csr_regs.mscratch;
}

int riscv_add_breakpoint (riscv_t * riscv, riscv_word_t addr) {
    breakpoint_t * breakpoint = (breakpoint_t *)calloc(1, sizeof(breakpoint_t));
    if (breakpoint == NULL) {
        return -1;
    }

    breakpoint->addr = addr;
    breakpoint->next = riscv->breaklist;
    riscv->breaklist = breakpoint;
    return 0;
}

void riscv_remove_breakpoint (riscv_t * riscv, riscv_word_t addr) {
    breakpoint_t * curr = riscv->breaklist;
    breakpoint_t * pre = NULL;
    while (curr) {
        if (curr->addr == addr) {
            if (pre == NULL) {
                riscv->breaklist = curr->next;
            } else {
                pre->next = curr->next;
            }
            free(curr);
            return;
        }

        pre = curr;
        curr = curr->next;
    }
}

/**
 * 复位内核
 */
void riscv_reset(riscv_t* riscv) {
    riscv->pc = 0;
    riscv->instr.raw = 0;
    riscv->dev_read = riscv->dev_write = riscv->dev_list;

    // 存储器不清空，只清空寄存器
    memset(riscv->regs, 0, sizeof(riscv->regs));
    riscv_csr_init(riscv);
// 测试用
#if 0
    for (int i = 0; i < 32; i++) {
        riscv->regs[i] = i;
    }
#endif
}

#define i_get_imm(instr)   \
    ((int32_t)(instr.i.imm11_0 | ((instr.i.imm11_0 & (1 << 11)) ? (0xFFFFF << 12) : 0)))

#define b_get_imm(instr) \
    ((int32_t)(((instr.b.imm_4_1 << 1) | (instr.b.imm_10_5 << 5) | (instr.b.imm_11 << 11) | \
    (instr.b.imm_12 ? (0xFFFFF << 12) : 0))))

#define j_get_imm(instr)    \
    ((int32_t)((instr.j.imm_10_1 << 1) | (instr.j.imm_11 << 11) | (instr.j.imm_19_12 << 12) | \
    ((instr.j.imm_20) ? (0xFFF << 20) : 0)))

#define u_get_imm(instr)   (instr.raw & 0xFFFFF000)

#define s_get_imm(instr)    \
    ((int32_t)((instr.s.imm4_0 << 0) | (instr.s.imm11_5 << 5) | ((instr.s.imm11_5 & (1 << 6) ) ? (0xFFFFFFFF << 11) : 0)))
#define s_extend_64(val)  (int64_t)((val & (1 << 31)) ? (val | 0xFFFFFFFF00000000) : val)

void riscv_continue(riscv_t * riscv, int forever) {
    do {
        riscv_word_t pc = riscv->pc;

        // 读取指令
        riscv_word_t * mem = (riscv_word_t *)riscv->flash->mem;
        if ((pc >= riscv->flash->device.end) || (pc < riscv->flash->device.base)) {
            goto cont_end;
        }

        // 指令单步执行
        riscv->instr.raw = mem[(riscv->pc - riscv->flash->device.base) >> 2];
        switch (riscv->instr.opcode) {
            case OP_BREAK: {
                switch (riscv->instr.i.funct3) {
                    case FUNC3_CSRRW: {
                        riscv_word_t csr = riscv->instr.i.imm11_0;
                        riscv_word_t old = riscv_read_csr(riscv, csr);
                        riscv_word_t new = riscv_read_reg(riscv, riscv->instr.i.rs1);
                        riscv_write_reg(riscv, riscv->instr.i.rd, old);
                        riscv_write_csr(riscv, csr, new);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    case FUNC3_CSRRS: {
                        riscv_word_t csr = riscv->instr.i.imm11_0;
                        riscv_word_t old = riscv_read_csr(riscv, csr);
                        riscv_word_t imm = riscv_read_reg(riscv, riscv->instr.i.rs1);
                        riscv_write_reg(riscv, riscv->instr.i.rd, old);
                        riscv_write_csr(riscv, csr, old | imm);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    case FUNC3_CSRRC: {
                        riscv_word_t csr = riscv->instr.i.imm11_0;
                        riscv_word_t old = riscv_read_csr(riscv, csr);
                        riscv_word_t imm = riscv_read_reg(riscv, riscv->instr.i.rs1);
                        riscv_write_reg(riscv, riscv->instr.i.rd, old);
                        riscv_write_csr(riscv, csr, old & ~imm);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    case FUNC3_CSRRWI: {
                        riscv_word_t csr = riscv->instr.i.imm11_0;
                        riscv_word_t old = riscv_read_csr(riscv, csr);
                        riscv_write_reg(riscv, riscv->instr.i.rd, old);
                        riscv_write_csr(riscv, csr, riscv->instr.i.rs1);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    case FUNC3_CSRRSI: {
                        riscv_word_t csr = riscv->instr.i.imm11_0;
                        riscv_word_t old = riscv_read_csr(riscv, csr);
                        riscv_write_reg(riscv, riscv->instr.i.rd, old);
                        riscv_write_csr(riscv, csr, old | riscv->instr.i.rs1);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    case FUNC3_CSRRCI: {
                        riscv_word_t csr = riscv->instr.i.imm11_0;
                        riscv_word_t old = riscv_read_csr(riscv, csr);
                        riscv_write_reg(riscv, riscv->instr.i.rd, old);
                        riscv_write_csr(riscv, csr, old & ~riscv->instr.i.rs1);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    default:
                        goto cont_end;
                }
                break;
            }
            case OP_LUI: {
                riscv_write_reg(riscv, riscv->instr.u.rd, u_get_imm(riscv->instr));
                riscv->pc += sizeof(instr_t);
                break;
            }
            case OP_AUIPC: {
                riscv_write_reg(riscv, riscv->instr.u.rd, riscv->pc + u_get_imm(riscv->instr));
                riscv->pc += sizeof(instr_t);
                break;
            }
            case OP_JAL: {
                int32_t imm  = j_get_imm(riscv->instr);
                riscv_write_reg(riscv, riscv->instr.u.rd, riscv->pc + 4);
                riscv->pc = (riscv_word_t)(riscv->pc + imm);
                break;
            }
            case OP_JALR: {
                riscv_word_t ret =riscv->pc;
                int32_t jump_addr = (int32_t)riscv_read_reg(riscv, riscv->instr.i.rs1) + i_get_imm(riscv->instr);
                riscv->pc = (riscv_word_t)jump_addr;
                riscv_write_reg(riscv, riscv->instr.i.rd,  ret + 4);
                break;
            }
            case OP_ADDI: {
                switch (riscv->instr.i.funct3) {
                    case FUNC3_ADDI: {
                        int32_t imm = (int32_t)(i_get_imm(riscv->instr)) + (int32_t)(riscv_read_reg(riscv, riscv->instr.i.rs1));;
                        riscv_write_reg(riscv, riscv->instr.i.rd, imm);
                        riscv->pc += sizeof(instr_t);
                        break;
                    }
                    case FUNC3_SLTI: {
                        riscv_word_t rs1 = riscv_read_reg(riscv, riscv->instr.i.rs1);
                        int32_t imm = i_get_imm(riscv->instr);
                        riscv_write_reg(riscv, riscv->instr.i.rd, (int32_t)rs1 < imm);
                        riscv->pc += sizeof(instr_t);
                        break;
                    }
                    case FUNC3_SLTIU: {
                        riscv_word_t rs1 = riscv_read_reg(riscv, riscv->instr.i.rs1);
                        riscv_word_t imm = i_get_imm(riscv->instr);  // 有符号扩展
                        riscv_write_reg(riscv, riscv->instr.i.rd, rs1 < imm);
                        riscv->pc += sizeof(instr_t);
                        break;
                    }
                    case FUNC3_XORI: {
                         riscv_word_t rs1 = riscv_read_reg(riscv, riscv->instr.i.rs1);
                         int32_t imm = i_get_imm(riscv->instr);
                         riscv_write_reg(riscv, riscv->instr.i.rd, imm ^ rs1);
                         riscv->pc += sizeof(instr_t);
                         break;
                    }
                    case FUNC3_ORI: {
                        riscv_word_t rs1 = riscv_read_reg(riscv, riscv->instr.i.rs1);
                        int32_t imm = i_get_imm(riscv->instr);
                        riscv_write_reg(riscv, riscv->instr.i.rd, imm | rs1);
                        riscv->pc += sizeof(instr_t);
                        break;
                    }
                    case FUNC3_ANDI: {
                        riscv_word_t rs1 = riscv_read_reg(riscv, riscv->instr.i.rs1);
                        int32_t imm = i_get_imm(riscv->instr);
                        riscv_write_reg(riscv, riscv->instr.i.rd, imm & rs1);
                        riscv->pc += sizeof(instr_t);
                        break;
                    }
                    case FUNC3_SLLI: {
                        riscv_word_t rs1 = riscv_read_reg(riscv, riscv->instr.i.rs1);
                        riscv_word_t imm = riscv->instr.i.imm11_0 & 0x001F;
                        riscv_write_reg(riscv, riscv->instr.i.rd, rs1 << imm);
                        riscv->pc += sizeof(instr_t);
                        break;
                    }
                    case FUNC3_SR: {
                        if (riscv->instr.raw & (1 << 30)) {
                            // srai
                            riscv_word_t rs1 = riscv_read_reg(riscv, riscv->instr.i.rs1);
                            riscv_word_t imm = riscv->instr.i.imm11_0 & 0x001F;
                            riscv_write_reg(riscv, riscv->instr.i.rd, (int32_t)rs1 >> imm);
                            riscv->pc += sizeof(instr_t);
                        } else {
                            // srli
                            riscv_word_t rs1 = riscv_read_reg(riscv, riscv->instr.i.rs1);
                            riscv_word_t imm = riscv->instr.i.imm11_0 & 0x001F;
                            riscv_write_reg(riscv, riscv->instr.i.rd, (riscv_word_t)rs1 >> imm);
                            riscv->pc += sizeof(instr_t);
                        }
                       break;
                    }
                    default:
                        goto cont_end;
                }
                break;
            }
            case OP_ADD: {
                switch (riscv->instr.r.funct3) {
                    case FUNC3_ADD: {
                        switch (riscv->instr.r.funct7) {
                            case FUNC7_ADD: {
                                int32_t result = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs1) + (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs2);
                                riscv_write_reg(riscv, riscv->instr.r.rd, result);
                                riscv->pc += sizeof(instr_t);
                                break;
                            }
                            case FUNC7_SUB: {
                                int32_t result = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs1) - (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs2);
                                riscv_write_reg(riscv, riscv->instr.r.rd, result);
                                riscv->pc += sizeof(instr_t);
                                break;
                            }
                            case FUNC7_MUL: {
                                riscv_word_t result = riscv_read_reg(riscv, riscv->instr.r.rs1) * riscv_read_reg(riscv, riscv->instr.r.rs2);
                                riscv_write_reg(riscv, riscv->instr.r.rd, result);
                                riscv->pc += sizeof(instr_t);
                                break;
                            }
                            default:
                                goto cont_end;
                        }
                        break;
                    }
                    case FUNC3_SLL: {
                        if (riscv->instr.r.funct7 == FUNC7_MUL) {
                            // MULH
                            int64_t rs1 = s_extend_64(riscv_read_reg(riscv, riscv->instr.r.rs1));
                            int64_t rs2 = s_extend_64(riscv_read_reg(riscv, riscv->instr.r.rs2));
                            riscv_write_reg(riscv, riscv->instr.r.rd, (rs1 * rs2) >> 32);
                            riscv->pc += sizeof(instr_t);
                        } else {
                            // SLL
                            riscv_word_t result = riscv_read_reg(riscv, riscv->instr.r.rs1) << riscv_read_reg(riscv, riscv->instr.r.rs2);
                            riscv_write_reg(riscv, riscv->instr.r.rd, result);
                            riscv->pc += sizeof(instr_t);
                            break;
                        }
                        break;
                    }
                    case FUNC3_SLT: {
                        if (riscv->instr.r.funct7 == FUNC7_MUL) {
                            // mulhsu
                            int64_t rs1 = s_extend_64(riscv_read_reg(riscv, riscv->instr.r.rs1));
                            uint64_t rs2 = riscv_read_reg(riscv, riscv->instr.r.rs2);
                            riscv_write_reg(riscv, riscv->instr.r.rd, (rs1 * rs2) >> 32);
                            riscv->pc += sizeof(instr_t);
                        } else {                        // slt
                            int result = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs1) < (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs2);
                            riscv_write_reg(riscv, riscv->instr.r.rd, result);
                            riscv->pc += sizeof(instr_t);
                        }
                        break;
                    }
                    case FUNC3_SLTU: {
                        if (riscv->instr.r.funct7 == FUNC7_MUL) {
                            // mulhu
                            uint64_t result = (uint64_t)riscv_read_reg(riscv, riscv->instr.r.rs1) * (uint64_t)riscv_read_reg(riscv, riscv->instr.r.rs2);
                            riscv_write_reg(riscv, riscv->instr.r.rd, result >> 32);
                            riscv->pc += sizeof(instr_t);
                        } else {                        // sltu
                            int result = riscv_read_reg(riscv, riscv->instr.r.rs1) < riscv_read_reg(riscv, riscv->instr.r.rs2);
                            riscv_write_reg(riscv, riscv->instr.r.rd, result);
                            riscv->pc += sizeof(instr_t);
                        }
                        break;
                    }
                    case FUNC3_XOR: {
                        if (riscv->instr.r.funct7 == FUNC7_DIV) {
                            // div
                            int32_t result = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs1) / (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs2);
                            riscv_write_reg(riscv, riscv->instr.r.rd, result);
                            riscv->pc += sizeof(instr_t);
                        } else {                        // xor
                            riscv_word_t result = riscv_read_reg(riscv, riscv->instr.r.rs1) ^ riscv_read_reg(riscv, riscv->instr.r.rs2);
                            riscv_write_reg(riscv, riscv->instr.r.rd, result);
                            riscv->pc += sizeof(instr_t);
                        }
                        break;
                    }
                    case FUNC3_SRL: {
                        switch (riscv->instr.r.funct7) {
                            case FUNC7_SRL: {
                                riscv_word_t result = riscv_read_reg(riscv, riscv->instr.r.rs1) >> riscv_read_reg(riscv, riscv->instr.r.rs2);
                                riscv_write_reg(riscv, riscv->instr.r.rd, result);
                                riscv->pc += sizeof(instr_t);
                                break;
                            }
                            case FUNC7_SRA: {
                                int32_t result = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs1) >> (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs2);
                                riscv_write_reg(riscv, riscv->instr.r.rd, result);
                                riscv->pc += sizeof(instr_t);
                                break;
                            }
                case FUNC7_DIV: {
                    // divu
                    riscv_word_t result = riscv_read_reg(riscv, riscv->instr.r.rs1) / riscv_read_reg(riscv, riscv->instr.r.rs2);
                    riscv_write_reg(riscv, riscv->instr.r.rd, result);
                    riscv->pc += sizeof(instr_t);
                    break;
                }                            default:
                                goto cont_end;
                        }
                        break;
                    }
                    case FUNC3_OR: {
                        if (riscv->instr.r.funct7 == FUNC7_REM) {
                            // rem
                            int32_t result = (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs1) % (int32_t)riscv_read_reg(riscv, riscv->instr.r.rs2);
                            riscv_write_reg(riscv, riscv->instr.r.rd, result);
                            riscv->pc += sizeof(instr_t);
                        } else {                        // or
                            riscv_word_t result = riscv_read_reg(riscv, riscv->instr.r.rs1) | riscv_read_reg(riscv, riscv->instr.r.rs2);
                            riscv_write_reg(riscv, riscv->instr.r.rd, result);
                            riscv->pc += sizeof(instr_t);
                        }
                        break;
                    }
                    case FUNC3_AND: {
                        if (riscv->instr.r.funct7 == FUNC7_REM) {
                            // remu
                            riscv_word_t result = riscv_read_reg(riscv, riscv->instr.r.rs1) % riscv_read_reg(riscv, riscv->instr.r.rs2);
                            riscv_write_reg(riscv, riscv->instr.r.rd, result);
                            riscv->pc += sizeof(instr_t);
                        } else {                        // and
                            riscv_word_t result = riscv_read_reg(riscv, riscv->instr.r.rs1) & riscv_read_reg(riscv, riscv->instr.r.rs2);
                            riscv_write_reg(riscv, riscv->instr.r.rd, result);
                            riscv->pc += sizeof(instr_t);
                        }
                        break;
                    }
                    default:
                        goto cont_end;
                }
                break;
            }
            case OP_BEQ: {
                switch (riscv->instr.b.funct3) {
                    case FUNC3_BEQ: {
                        if(riscv_read_reg(riscv, riscv->instr.b.rs1) == riscv_read_reg(riscv, riscv->instr.b.rs2)) {
                            riscv->pc = (int32_t)pc + b_get_imm(riscv->instr);
                        } else {
                            riscv->pc += sizeof(instr_t);
                        }
                        break;
                    }
                    case FUNC3_BNE: {
                        if(riscv_read_reg(riscv, riscv->instr.b.rs1) != riscv_read_reg(riscv, riscv->instr.b.rs2)) {
                            riscv->pc = (int32_t)pc + b_get_imm(riscv->instr);
                        } else {
                            riscv->pc += sizeof(instr_t);
                        }
                        break;
                    }
                    case FUNC3_BLT: {
                        if((int32_t)riscv_read_reg(riscv, riscv->instr.b.rs1) < (int32_t)riscv_read_reg(riscv, riscv->instr.b.rs2)) {
                            riscv->pc = (int32_t)pc + b_get_imm(riscv->instr);
                        } else {
                            riscv->pc += sizeof(instr_t);
                        }
                        break;
                    }
                    case FUNC3_BGE: {
                        if((int32_t)riscv_read_reg(riscv, riscv->instr.b.rs1) >= (int32_t)(riscv_read_reg(riscv, riscv->instr.b.rs2))) {
                            riscv->pc = (int32_t)riscv->pc + b_get_imm(riscv->instr);
                        } else{
                            riscv->pc += sizeof(riscv_word_t);
                        }
                        break;
                    }
                    case FUNC3_BLTU: {
                        if(riscv_read_reg(riscv, riscv->instr.b.rs1) < riscv_read_reg(riscv, riscv->instr.b.rs2)) {
                            riscv->pc = (int32_t)pc + b_get_imm(riscv->instr);
                        } else {
                            riscv->pc += sizeof(instr_t);
                        }
                        break;
                    }
                    case FUNC3_BGEU: {
                        if(riscv_read_reg(riscv, riscv->instr.b.rs1) >= riscv_read_reg(riscv, riscv->instr.b.rs2)) {
                            riscv->pc = (int32_t)riscv->pc + b_get_imm(riscv->instr);
                        } else{
                            riscv->pc += sizeof(riscv_word_t);
                        }
                        break;
                    }
                    default:
                        goto cont_end;
                }
                break;
            }
            case OP_SB: {
                switch (riscv->instr.i.funct3) {
                    case FUNC3_SB: {
                        riscv_word_t addr = riscv_read_reg(riscv, riscv->instr.s.rs1) + s_get_imm(riscv->instr);
                        riscv_word_t val = riscv_read_reg(riscv, riscv->instr.s.rs2);
                        int rc = riscv_mem_write(riscv, addr, (uint8_t *)&val, 1);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    case FUNC3_SH:{
                        riscv_word_t addr = riscv->regs[riscv->instr.s.rs1] + s_get_imm(riscv->instr);
                        riscv_word_t val = riscv_read_reg(riscv, riscv->instr.s.rs2);
                        int rc = riscv_mem_write(riscv, addr, (uint8_t *)&val, 2);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    case FUNC3_SW: {
                        riscv_word_t addr = riscv->regs[riscv->instr.s.rs1] + s_get_imm(riscv->instr);
                        riscv_word_t val = riscv_read_reg(riscv, riscv->instr.s.rs2);
                        int rc = riscv_mem_write(riscv, addr, (uint8_t *)&val, 4);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    default:
                        goto cont_end;
                }
                break;
            }
            case OP_LB: {
                switch (riscv->instr.i.funct3) {
                    case FUNC3_LB: {
                        riscv_word_t addr = riscv_read_reg(riscv, riscv->instr.i.rs1) + i_get_imm(riscv->instr);
                        riscv_word_t val = 0;
                        int rc = riscv_mem_read(riscv, addr, (uint8_t *)&val, 1);
                        riscv_write_reg(riscv, riscv->instr.i.rd, (val & (1 << 7)) ? (val | 0xFFFFFF00) : val);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    case FUNC3_LH: {
                        riscv_word_t addr = riscv_read_reg(riscv, riscv->instr.i.rs1) + i_get_imm(riscv->instr);
                        riscv_word_t val = 0;
                        int rc = riscv_mem_read(riscv, addr, (uint8_t *)&val, 2);
                        riscv_write_reg(riscv, riscv->instr.i.rd, (val & (1 << 15)) ? (val | 0xFFFF0000) : val);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    case FUNC3_LW: {
                        riscv_word_t addr = riscv_read_reg(riscv, riscv->instr.i.rs1) + i_get_imm(riscv->instr);
                        riscv_word_t val = 0;
                        int rc = riscv_mem_read(riscv, addr, (uint8_t *)&val, 4);
                        riscv_write_reg(riscv, riscv->instr.i.rd, val);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    case FUNC3_LBU: {
                        riscv_word_t addr = riscv_read_reg(riscv, riscv->instr.i.rs1) + i_get_imm(riscv->instr);
                        riscv_word_t val = 0;
                        int rc = riscv_mem_read(riscv, addr, (uint8_t *)&val, 1);
                        riscv_write_reg(riscv, riscv->instr.i.rd, val);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    case FUNC3_LHU: {
                        riscv_word_t addr = riscv_read_reg(riscv, riscv->instr.i.rs1) + i_get_imm(riscv->instr);
                        riscv_word_t val = 0;
                        int rc = riscv_mem_read(riscv, addr, (uint8_t *)&val, 2);
                        riscv_write_reg(riscv, riscv->instr.i.rd, val);
                        riscv->pc += sizeof(riscv_word_t);
                        break;
                    }
                    default:
                        goto cont_end;
                }
                break;
            }
            default:
                goto cont_end;
        }

        // 断点的处理
        if (forever) {
            breakpoint_t * curr = riscv->breaklist;
            while (curr) {
                if (curr->addr == riscv->pc) {
                    goto cont_end;
                }
                curr = curr->next;
            }

            // 检查是否有停止请求
            char c = EOF;
            recv(riscv->server->client, &c, 1, 0);
            if (c == 3) {
                break;
            }
        }

    } while (forever);

cont_end:
    return;
}

/**
 * 启动RISC-V内核运行
*/
void riscv_run (riscv_t * riscv) {
    riscv_reset(riscv);

    if (riscv->server) {
        gdb_server_run(riscv->server);
    } else {
        riscv_continue(riscv, 1);
    }
}

void riscv_load_bin (riscv_t * riscv, const char * file_name) {
    FILE * file = fopen(file_name, "rb");
    if (file == NULL) {
        fprintf(stderr, "open file %s failed.\n", file_name);
        exit(-1);
    }

    if (feof(file)) {
        fprintf(stderr, "file %s is empty.\n", file_name);
        exit(-1);
    }

    char buf[1024];
    char * dest = riscv->flash->mem;
    size_t size;
    while ((size = fread(buf, 1, sizeof(buf), file)) > 0) {
        memcpy(dest, buf, size);
        dest += size;
    }

    fclose(file);
}
