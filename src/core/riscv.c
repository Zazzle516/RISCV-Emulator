#include "instr_implements.h"
#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>

riscv_t* riscv_create(void) {
    riscv_t* riscv = (riscv_t*)calloc(1, sizeof(riscv_t));    // 因为要求返回指针 所以分配一个空间就可以    32 + 32 + 32*32 / 144
    assert(riscv != NULL);  // 判断为 True 继续运行
    return riscv;
}

// 挂载 flash 结构体
void riscv_flash_set(riscv_t* riscv, mem_t* flash) {
    riscv->riscv_flash = flash;
}

// 读取 image.bin 文件
void riscv_load_bin(riscv_t* riscv, const char* file_name) {
    // 相对路径在 launch.json 文件定义的根路径中
    // 注意指定读取方式 以二进制 b 的形式读取
    FILE* file = fopen(file_name, "rb");

    // 判断文件是否存在
    if (file == NULL) {
        fprintf(stderr, "file %s doesn't exist\n", file_name);
        exit(-1);
    }

    // 判断文件是否为空 程序报错退出
    if (feof(file)) {
        fprintf(stderr, "file %s is empty\n", file_name);
        exit(-1);
    }

    // 确认文件成功打开后写入数据(有缓冲区的方式)
    char buffer[1024];
    char* destination = riscv->riscv_flash->mem;    // 以写入的数据大小作为类型
    
    size_t size;

    // 之前我这里的 size 赋值少加了括号 然后优先级判定 size = 1 出了 bug
    while ((size = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        // 判断大于 0 只是判断仍然有内容没有写入 所以不一定每次都是 1024
        memcpy(destination, buffer, size);
        destination += size;
    }

    // 记得关闭
    fclose(file);
}

// 重置芯片状态
void riscv_reset(riscv_t* riscv) {
    riscv->pc = 0;      // 因为 main.c 中定义的 RISCV_FLASH_START 是从 0 开始
    memset(riscv->regs, 0, sizeof(riscv->regs));

    // 并没有重置 Flash 和 RAM

    // 添加了指令结构体之后也重置指令
    riscv->instr.raw = 0;

    // 重置 device_buffer
    riscv->device_read_buffer = riscv->device_list;
    riscv->device_write_buffer = riscv->device_list;
}

// 添加不同外部设备
void riscv_device_add(riscv_t* riscv, riscv_device_t* dev){
    // 使用头插法完成设备的添加
    dev->next = riscv->device_list;     // 首先指向头结点

    riscv->device_list = dev;           // 重新链接起来

    // 初始化 device_buffer
    if (riscv->device_read_buffer == NULL) {
        riscv->device_read_buffer = dev;
    }
    if (riscv->device_write_buffer == NULL) {
        riscv->device_write_buffer = dev;
    }
}

// 取值 -> 分析指令 -> 执行指令 流程模拟(核心)
void riscv_continue(riscv_t* riscv, int forever) {
    // 取指 riscv->pc; 

    // 预处理: 获取指向 image.bin 空间的指针
    // Q: 这个地方的强制类型转换是为什么    虽然它们的类型确实不一样    不能直接用 uin8_t* 接收吗
    riscv_word_t* mem = (riscv_word_t*) riscv->riscv_flash->mem;
    
    // 对执行的指令进行判断
    do
    {
        // 判断指令的地址合法性
        if (riscv->pc < riscv->riscv_flash->riscv_dev.addr_start || riscv->pc > riscv->riscv_flash->riscv_dev.addr_end) {
            fprintf(stderr, "Illegal Instruction Address\n");
            return;
        }

        // 根据 pc 得到指令的具体内容   单个指令的单位长度为 4B    这里通过下标访问
        riscv_word_t instruc = mem[(riscv->pc - riscv->riscv_flash->riscv_dev.addr_start) >> 2];
        
        // 也可以直接访问 pc
        // riscv_word_t instruc = (riscv_word_t *) riscv->pc;   直接访问肯定不行 因为访问的是 0x0000 是本机的地址 一定会被 OS 阻止 所以一定要加上 mem 的首地址
        // riscv_word_t instruc = (riscv_word_t *)(riscv->pc + mem);

        // 把指令放到 IR 中
        riscv->instr.raw = instruc;

        switch (riscv->instr.opcode)
        {
        case OP_BREAK:
            fprintf(stdout, "found ebreak!\n");
            return;

        case OP_ADDI: {
            switch (riscv->instr.i.funct3){
                case FUNC3_ADDI: {
                    handle_addi(riscv);
                    break;
                }
                case FUNC3_SLTI: {
                    handle_slti(riscv);
                    break;
                }
                case FUNC3_SLTIU: {
                    handle_sltiu(riscv);
                    break;
                }
                case FUNC3_XORI: {
                    handle_xori(riscv);
                    break;
                }
                case FUNC3_ORI: {
                    handle_ori(riscv);
                    break;
                }
                case FUNC3_ANDI: {
                    handle_andi(riscv);
                    break;
                }
                case FUNC3_SLLI: {
                    handle_slli(riscv);
                    break;
                }

                // 注意手册上 SRLI 和 SRAI 是两个指令 但是它们的 funct3 是相同的 所以用一条指令 SR 表示
                case FUNC3_SR: {
                    handle_srai_srli(riscv);
                    break;
                }

                default:
                    goto cond_end;
            }
            break;
        }

        case OP_ADD: {
            switch (riscv->instr.r.funct3)
            {
                case FUNC3_ADD: {
                    switch (riscv->instr.r.funct7)
                    {
                    case FUNC7_ADD:
                        handle_add(riscv);
                        break;
                    case FUNC7_SUB:
                        handle_sub(riscv);
                        break;
                    default:
                        goto cond_end;
                    }
                    break;
                }
                case FUNC3_SLL:
                    handle_sll(riscv);
                    break;
                case FUNC3_SLT:
                    handle_slt(riscv);
                    break;
                case FUNC3_SLTU:
                    handle_sltu(riscv);
                    break;
                case FUNC3_XOR:
                    handle_xor(riscv);
                    break;
                case FUNC3_SR:
                    handle_srl_sra(riscv);
                    break;
                case FUNC3_OR:
                    handle_or(riscv);
                    break;
                case FUNC3_AND:
                    handle_and(riscv);
                    break;
                default:
                    goto cond_end;
                }
            break;
        }
        
        case OP_LUI: {
            handle_lui(riscv);
            break;
        }

        case OP_SB: {
            switch (riscv->instr.s.funct3)
            {
            case FUNC3_SB:
                handle_sb(riscv);
                break;
            case FUNC3_SH:
                handle_sh(riscv);
                break;
            case FUNC3_SW:
                handle_sw(riscv);
                break;
            default:
                goto cond_end;
            }
            break;
        }

        case OP_LB: {
            switch (riscv->instr.i.funct3)
            {
            case FUNC3_LB:
                handle_lb(riscv);
                break;
            case FUNC3_LH:
                handle_lh(riscv);
                break;
            case FUNC3_LW:
                handle_lw(riscv);
                break;
            case FUNC3_LBU:
                handle_lbu(riscv);
                break;
            case FUNC3_LHU:
                handle_lhu(riscv);
                break;
            default:
                goto cond_end;
            }
            break;
        }
        
        default:
            goto cond_end;
        }
    } while (forever);
    return;

cond_end:
    fprintf(stderr, "Unable to recognize %x\n", riscv->instr.raw);
}

// 遍历外设链表 根据地址找到对应设备
riscv_device_t* device_find(riscv_t* riscv, riscv_word_t addr) {
    // 在有 device_buffer 的情况下仍然没有找到 必须进行查找
    riscv_device_t* dev = riscv->device_list;
    while (dev) {
        if (addr >= dev->addr_start && addr < dev->addr_end) {
            return dev;
        }
        dev = dev->next;
    }
    return NULL;
}

// 从模拟器的角度找到读写区域对应的设备 根据设备的特性去调用读写函数
int riscv_mem_read(riscv_t* riscv, riscv_word_t start_addr, uint8_t* val, int width) {
    // 在实际的 read 操作中根据 device_buffer 优化
    if (start_addr >= riscv->device_read_buffer->addr_start && start_addr < riscv->device_read_buffer->addr_end) {
        return riscv->device_read_buffer->read(riscv->device_read_buffer, start_addr, val, width);
    }
    riscv_device_t* targetDevice = device_find(riscv, start_addr);
    
    if (targetDevice == NULL) {
        fprintf(stderr, "invaild read address\n");
        return -1;
    }
    
    // 更新缓存
    riscv->device_read_buffer = targetDevice;

    // 注意 val 传入的是地址 因为你不知道用户要读取几个字节 所以以 1B 为单位 从首地址开始处理
    return targetDevice->read(targetDevice, start_addr, val, width);
}

int riscv_mem_write(riscv_t* riscv, riscv_word_t start_addr, uint8_t* val, int width) {
    if (start_addr >= riscv->device_write_buffer->addr_start && start_addr < riscv->device_write_buffer->addr_end) {
        return riscv->device_write_buffer->read(riscv->device_write_buffer, start_addr, val, width);
    }
    riscv_device_t* targetDevice = device_find(riscv, start_addr);
    
    if (targetDevice == NULL) {
        fprintf(stderr, "current instr: %x\n", riscv->instr.raw);
        fprintf(stderr, "invaild write address\n");
        return -1;
    }
    
    riscv->device_write_buffer = targetDevice;

    return targetDevice->write(targetDevice, start_addr, val, width);
}
