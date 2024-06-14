#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>

#include "riscv.h"

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
}

// 符号扩展: 把 12 位扩展到 20 位
#define i_get_imm(instr)       \
    ((int32_t)(instr.i.imm11_0 | ((instr.i.imm11_0 & (1 << 11)) ? (0xFFFFF << 12) : 0)))

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
            switch (riscv->instr.i.funct3)
            {
            case FUNC3_ADDI: {
                int32_t source = (int32_t)riscv_read_reg(riscv, riscv->instr.i.rs1);
                int32_t imm = (int32_t)i_get_imm(riscv->instr);
                int32_t result = imm + source;
                riscv_write_reg(riscv, riscv->instr.i.rd, result);
                
                riscv->pc += sizeof(riscv_word_t);
                break;
            }

            default:
                goto cond_end;
            }
            break;
        }

        default:
            goto cond_end;
            fprintf(stdout, "normal instruction: %x\n", riscv->instr.raw);

            // 更新 pc 指向
            riscv->pc += sizeof(riscv_word_t);
            break;
        }
    } while (forever);
    return;

cond_end:
    fprintf(stderr, "Unable to recognize %x\n", riscv->instr.raw);
}

// 语法解决部分
int riscv_mem_read(riscv_t* riscv, riscv_word_t start_addr, uint8_t* val, int width){
    return 0;
}
