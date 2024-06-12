#ifndef INSTR_TEST_H
#define INSTR_TEST_H

#include "plat/plat.h"

#define PATH_PRE    "./unit/"

#define assert_int_equal(a, b)  \
    if (a != b) { \
        fprintf(stderr, "assert failed: %s(%d) != %s(%d) (%s: %d)\n", #a, a, #b, b, __FILE__, __LINE__); \
        exit(-1); \
    }

static void run_to_ebreak (riscv_t * riscv) {
    while (1) {
        riscv_word_t pc = riscv->pc;
        riscv_continue(riscv, 0);
        if (riscv->instr.raw == EBREAK) {
            break;
        }
    }
}

static void check_reg (riscv_t * riscv, riscv_word_t * reglist, int cnt) {
    for (int i = 0; i < cnt; i++) {
        riscv_word_t reg = riscv_read_reg(riscv, i);
        if (reg != reglist[i]) {
            fprintf(stderr, "reg %d is not correct: real %x != expect %x", i, reg, reglist[i]);
            if (reg != reglist[i]) {
                exit(-1);
            }
        }
    }
}

static void test_riscv_ebreak (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"ebreak/obj/image.bin");
    riscv_reset(riscv);    
    run_to_ebreak(riscv);
    assert_int_equal(riscv->pc, 0);
}

static void test_riscv_addi (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"00_addi/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x00, 0x11, 0x33, 0x66, 0xaa, 0xff, 0x165, 0x1dc, 0x264, 0x2fd, 0x3fd, 0x50e, 
        0x630, 0x763, 0x8a7, 0x9fc, 0xb65, 0xcd5, 0xe55, 0xfe5, 0x11e5, 0x13f5,
        0x1615, 0x1845, 0x1a85, 0x1cd5, 0x1f35, 0x21a5, 0x2425, 0x26b5, 0x2ab3, 0x26b3
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_ori (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"01_ori/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x000, 0x111, 0x133, 0x133, 0x155, 0x175, 0x177, 0x177,0x199, 0x199, 0x1bb, 0x1bb, 0x1dd, 0x1dd, 0x1ff, 
        0x1ff, 0x111, 0x333, 0x333, 0x555, 0x555, 0x777, 0x777, 0x311, 0x333, 0x333, 0x355, 0x355, 0x377, 
        0x377, 0x111, 0xfffffbdb
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_add (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"02_add/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x0, 0x11, 0xfffffc12, 0xfffffc23, 0xfffff835,
        0xfffff458, 0xffffec8d, 0xffffe0e5, 0xffffcd72,
        0xffffae57, 0x0, 0xffffae57, 0xffffae57,
        0xffff5cae, 0xffff0b05, 0xfffe67b3, 0xfffd72b8,
        0xfffbda6b, 0xfff94d23, 0xfff5278e, 0xffee74b1,
        0xffe39c3f, 0xffd210f0, 0xffb5ad2f, 0xff87be1f,
        0xff3d6b4e, 0xfec5296d, 0xfe0294bb, 0xfcc7be28,
        0xfaca52e3, 0xf792110b, 0xf25c63ee
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_slli (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"03_slli/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x00000000, 0x7e635776, 0xfcc6aeec, 0xf98d5dd8, 0xf31abbb0, 0xe6357760, 0xcc6aeec0, 0x98d5dd80, 
        0x31abbb00, 0x63577600, 0xc6aeec00, 0x8d5dd800, 0x1abbb000, 0x35776000, 0x6aeec000, 0xd5dd8000, 
        0xabbb0000, 0x57760000, 0xaeec0000, 0x5dd80000, 0xbbb00000, 0x77600000, 0xeec00000, 0xdd800000, 
        0xbb000000, 0x76000000, 0xec000000, 0xd8000000, 0xb0000000, 0x60000000, 0xc0000000, 0x80000000
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_srli (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"04_srli/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x0, 0x1f98d5dd, 0xfcc6aee, 0x7e63577,
        0x3f31abb, 0x1f98d5d, 0xfcc6ae, 0x7e6357,
        0x3f31ab, 0x1f98d5, 0xfcc6a, 0x7e635,
        0x3f31a, 0x1f98d, 0xfcc6, 0x7e63,
        0x3f31, 0x1f98, 0xfcc, 0x7e6,
        0x3f3, 0x1f9, 0xfc, 0x7e,
        0x3f, 0x1f, 0xf, 0x7,
        0x3, 0x1, 0x3f31abbb, 0x0
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_srai (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"05_srai/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x0, 0xfffffc3a, 0xfffffe1d, 0xffffff0e,
        0xffffff87, 0xffffffc3, 0xffffffe1, 0xfffffff0,
        0xfffffff8, 0xfffffffc, 0xfffffffe, 0xffffffff,
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
        0x7e63, 0x3f31, 0x1f98, 0xfcc,
        0x7e6, 0x3f3, 0x1f9, 0xfc,
        0x7e, 0x3f, 0x1f, 0xf,
        0x7, 0x3, 0x7e6354bb, 0x0
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_andi (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"06_andi/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x0, 0x1, 0x202, 0x203,
        0x40, 0x41, 0x42, 0x43,
        0x0, 0x1, 0x2, 0x3,
        0x40, 0x41, 0x42, 0x43,
        0xfffffa80, 0xfffffa90, 0xfffff810, 0xfffffa80,
        0xfffff800, 0xfffff810, 0xfffffa10, 0xfffffa10,
        0xfffffa90, 0xfffffa90, 0xfffffa00, 0xfffff880,
        0xfffffa10, 0xfffff880, 0xfffff890, 0xfffff810
    };

    check_reg(riscv, reglist, 32);
}
static void test_riscv_slti (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"07_slti/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x0, 0x234, 0x1, 0x0, 0xfffffdcc, 0x0,
        0x1, 0x0, 0x1, 0x1, 0x1,
        0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x1, 0x1, 0x1, 0x0,
        0x0, 0x1, 0x0, 0x0, 0x0,
        0x1, 0x1, 0x1, 0x1, 0x1, 0x1,
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_sltiu (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"08_sltiu/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x0, 0x234, 0x1, 0x0, 0xfffffdcc, 0x0,
        0x1, 0x0, 0x1, 0x1, 0x1,
        0x0, 0x0, 0x0, 0x1, 0x1,
        0x0, 0x1, 0x1, 0x1, 0x1,
        0x1, 0x1, 0x1, 0x1, 0x0,
        0x1, 0x1, 0x1, 0x1, 0x0, 0x1,
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_xori (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"09_xori/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0, 0x352, 0x61, 0x170, 0x207, 0x226,
        0x225, 0x234, 0x2cb, 0x2da, 0x2e9,
        0x2f8, 0x28f, 0x29e, 0x2ad, 0x2bc,
        0x570, 0x545, 0x2e2, 0x537, 0x7b8,
        0x6c1, 0x5ef, 0x5cd, 0x427, 0x42f,
        0x4da, 0x71f, 0x4c0, 0x773, 0x70c,
        0x2e4,
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_sub (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"10_sub/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x00000000,0x11, 0xfffffc12, 0xfffffc01, 0xffffffef, 0x3ee,
        0x3ff, 0x11, 0xfffffc12, 0xfffffc01, 0xffffffef,
        0x3ee, 0x3ff, 0x11, 0xfffffc12, 0xfffffc01,
        0xffffffef, 0x3ee, 0x3ff, 0x11, 0xfffffc12,
        0xfffffc01, 0xffffffef, 0x3ee, 0x3ff, 0x11,
        0xfffffc12, 0xfffffc01, 0xffffffef, 0x3ee, 0x3ff,
        0x11,
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_sll (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"11_sll/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x00000000, 0x1, 0x2, 0x3, 0x4, 0x5,
        0x6, 0x7, 0x8, 0x9, 0xa,
        0xb, 0xc, 0xd, 0xe, 0xf,
        0xfffff0e8, 0xffffe1d0, 0xffffc3a0, 0xffff8740, 0xffff0e80,
        0xfffe1d00, 0xfffc3a00, 0xfff87400, 0xfff0e800, 0xffe1d000,
        0xffc3a000, 0xff874000, 0xff0e8000, 0xfe1d0000, 0xfc3a0000,
        0xfff87400,
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_slt (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"12_slt/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x00000000, 0x234, 0x345, 0x0, 0xfffffdcc, 0xfffffcbb,
        0xfffffdd0, 0x1, 0x3bc, 0x234, 0x345,
        0xfffffcbb, 0xfffffdcc, 0x230, 0xfffffdd0, 0xfffffdd0,
        0x1, 0x1, 0x0, 0x0, 0x1,
        0x0, 0x0, 0x1, 0x0, 0x1,
        0x1, 0x0, 0x1, 0x0, 0x1, 0x1,
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_sltu (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"13_sltu/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x00000000, 0x234, 0x345, 0x0, 0xfffffdcc, 0xfffffcbb,
        0xfffffdd0, 0x1, 0x3bc, 0x234, 0x345,
        0xfffffcbb, 0xfffffdcc, 0x230, 0xfffffdd0, 0xfffffdd0,
        0x1, 0x1, 0x1, 0x1, 0x0,
        0x1, 0x0, 0x0, 0x0, 0x1,
        0x0, 0x0, 0x0, 0x0, 0x0,
        0x1,
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_xor (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"14_xor/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x00000000, 0x11, 0xfffffc12, 0xfffffc03, 0x11, 0xfffffc12,
        0xfffffc03, 0x11, 0xfffffc12, 0xfffffc03, 0x0,
        0xfffffc03, 0xfffffc03, 0x0, 0xfffffc03, 0xfffffc03,
        0x0, 0xfffffc03, 0xfffffc03, 0x0, 0xfffffc03,
        0xfffffc03, 0x0, 0xfffffc03, 0xfffffc03, 0x0,
        0xfffffc03, 0xfffffc03, 0x0, 0xfffffc03, 0xfffffc03, 0,
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_srl (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"15_srl/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x00000000, 0x1, 0x2, 0x3, 0x4, 0x5,
        0x6, 0x7, 0x8, 0x9, 0xa,
        0xb, 0xc, 0xd, 0xe, 0xf,
        0x269, 0x134, 0x9a, 0x4d, 0x26,
        0x13, 0x9, 0x4, 0x7ffffd, 0x3ffffe,
        0x1fffff, 0xfffff, 0x7ffff, 0x3ffff, 0x1ffff,
        0x7ffffd,
    };

    check_reg(riscv, reglist, 32);
}


static void test_riscv_sra (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"16_sra/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x00000000, 0x1, 0x2, 0x3, 0x4, 0x5,
        0x6, 0x7, 0x8, 0x9, 0xa,
        0xb, 0xc, 0xd, 0xe, 0xf,
        0x269, 0x134, 0x9a, 0x4d, 0x26,
        0x13, 0x9, 0x4, 0xfffffffd, 0xfffffffe,
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
        0xfffffffd,
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_or (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"17_or/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x00000000, 0x1, 0x2, 0x3, 0x4, 0x5,
        0x6, 0x7, 0x8, 0x9, 0xa,
        0xb, 0xc, 0xd, 0xe, 0xf,
        0x4d3, 0x4d2, 0x4d3, 0x4d6, 0x4d7,
        0x4d6, 0x4d7, 0x4da, 0xfffffb2f, 0xfffffb2e,
        0xfffffb2f, 0xfffffb2e, 0xfffffb2f, 0xfffffb2e, 0xfffffb2f,
        0xffffffff,
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_sb (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"18_sb/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    // 读取内存内容并验证
    riscv_word_t content;
    riscv_mem_read(riscv, 0x20000000, (uint8_t *)&content, sizeof(riscv_word_t));
    assert_int_equal(content, 0xb3b2b1b0);
    riscv_mem_read(riscv, 0x20000014, (uint8_t *)&content, sizeof(riscv_word_t));
    assert_int_equal(content, 0x23451234);
    riscv_mem_read(riscv, 0x20000018, (uint8_t *)&content, sizeof(riscv_word_t));
    assert_int_equal(content, 0x12345678);
    riscv_mem_read(riscv, 0x2000001c, (uint8_t *)&content, sizeof(riscv_word_t));
    assert_int_equal(content, 0x87654321);
    riscv_mem_read(riscv, 0x20002000, (uint8_t *)&content, sizeof(riscv_word_t));
    assert_int_equal(content, 0x11223344);
}

static void test_riscv_lb (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"19_lb/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[16] = {
        0, 0x87a593b1, 0x20000000, 0xffffffb1,
        0xffffff93, -91, -121, -27727,
        0xffff87a5, -2019191887, 177, 147,
        165, 135, 37809, 34725,
    };

    check_reg(riscv, reglist, 16);
}

static void test_riscv_lui (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"20_lui/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x00000000u, 0x01111000u, 0x02222000u, 0x03333000u,
        0x04444000u, 0x05555000u, 0x06666000u, 0x07777000u,
        0x08888000u, 0x09999000u, 0x01010000u, 0x01111000u,
        0x01212000u, 0x01313000u, 0x01414000u, 0x01515000u,
        0x01616000u, 0x01717000u, 0x01818000u, 0x01919000u,
        0x02020000u, 0x02121000u, 0x02222000u, 0x02323000u,
        0x02424000u, 0x02525000u, 0x02626000u, 0x02727000u,
        0x02828000u, 0x02929000u, 0x03030000u, 0x03223000u
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_auipc (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"21_auipc/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0x00000000, 0x1111000, 0x2222004, 0x3333008, // zero, ra, sp, gp
        0x444400c, 0x5555010, 0x6666014, 0x7777018, // tp, t0, t1, t2
        0x888801c, 0x9999020, 0x1010024, 0x1111028, // fp, s1, a0, a1
        0x121202c, 0x1313030, 0x1414034, 0x1515038, // a2, a3, a4, a5
        0x161603c, 0x1717040, 0x1818044, 0x1919048, // a6, a7, s2, s3
        0x202004c, 0x2121050, 0x2222054, 0x2323058, // s4, s5, s6, s7
        0x242405c, 0x2525060, 0x2626064, 0x2727068, // s8, s9, s10, s11
        0x282806c, 0x2929070, 0x3030074, 0x3223078  // t3, t4, t5, t6
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_and (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"22_and/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);

    riscv_word_t reglist[32] = {
        0, 0x332, 0xffffff33, 0x332, 0x332, 0x332,
        0x332, 0x332, 0x332, 0x332, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0,
        0x0,
    };

    check_reg(riscv, reglist, 32);
}

static void test_riscv_jal (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"23_jal/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 单步调试，观察其运行流程
    riscv_continue(riscv, 0);  // jal x1, next
    assert_int_equal(riscv->pc, 0x80c);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x404);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0xc14);

    riscv_word_t reglist[] = {
        0x00000000, // zero
        0x00000004, // ra
        0x00000100, // sp
        0x00000200, // gp
        0x00000814, // tp
        0x0000040c, // tp
    };

    check_reg(riscv, reglist, 6);    
}

static void test_riscv_jalr (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"24_jalr/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 单步调试，观察其运行流程
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x18);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x24);
    
    run_to_ebreak(riscv);
    assert_int_equal(riscv->pc, 0x14);

    riscv_word_t reglist[] = {
        0x00000000, // zero
        0x00000018, // ra
        0x00000014, // sp
        0x00000100, // gp
        0x00000101, // tp
        0x00000102, // tp
    };

    check_reg(riscv, reglist, 6);    
}

static void test_riscv_beq (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"25_beq/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 单步调试，观察其运行流程
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x18);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x14); 
}

static void test_riscv_bne (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"26_bne/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 单步调试，观察其运行流程
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x18);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x14); 
}

static void test_riscv_bltu (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"27_bltu/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x14);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x10);
}

static void test_riscv_bgeu (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"28_bgeu/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x14);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x10);
}

static void test_riscv_blt (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"29_blt/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x14);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x10);
}

static void test_riscv_bge (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"30_bge/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x14);
    riscv_continue(riscv, 0);
    riscv_continue(riscv, 0);
    assert_int_equal(riscv->pc, 0x10);
}

static void test_riscv_mul (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"31_mul/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);
    riscv_word_t reglist[12] = {
        0x0, 0x324487, 0x87654321, 0x70b88d78, 0x2, 0x690302d1,
        0x596298d, 0xff9459e2, 0x19db187, 0x9a0cd05, 0xffe569fa,
        0x1a9605
    };

    check_reg(riscv, reglist, 12);
}

static void test_riscv_div (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"32_div/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);
    riscv_word_t reglist[11] = {
        0x0, 0x1e854a9, 0x4, 0x8, 0xfffffff8, 0x8,
        0x13880, 0xfffffffd, 0xfffffffd, 0x3, 0x1,
    };

    check_reg(riscv, reglist, 11);
}

static void test_riscv_csr (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"33_csr/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);
    riscv_word_t reglist[7] = {
        0x00000000, 0x12345736, 0x54321356, 0x12345678, 0x12345736, 0x56365776, 0x2044420,
    };

    check_reg(riscv, reglist, 7);
}

static void test_riscv_csri (riscv_t * riscv) {
    riscv_load_bin(riscv, PATH_PRE"34_csri/obj/image.bin");

    // 复位内核
    riscv_reset(riscv);

    // 顺序执行
    run_to_ebreak(riscv);
    riscv_word_t reglist[4] = {
        0x00000000, 0x1A, 0x1B, 0x10
    };

    check_reg(riscv, reglist, 4);
}

#define UNIT_TEST(f)        {#f, f}

void instr_test (riscv_t * riscv) {
    static const struct {
        const char * name;
        void (*test_func)(riscv_t * riscv);
    } tests[] = {
        UNIT_TEST(test_riscv_ebreak),
        UNIT_TEST(test_riscv_addi),
        UNIT_TEST(test_riscv_ori),
        UNIT_TEST(test_riscv_add),
        UNIT_TEST(test_riscv_slli),
        UNIT_TEST(test_riscv_srli),
        UNIT_TEST(test_riscv_srai),
        UNIT_TEST(test_riscv_andi),
        UNIT_TEST(test_riscv_slti),
        UNIT_TEST(test_riscv_sltiu),
        UNIT_TEST(test_riscv_xori),
        UNIT_TEST(test_riscv_sub),
        UNIT_TEST(test_riscv_sll),
        UNIT_TEST(test_riscv_slt),
        UNIT_TEST(test_riscv_sltu),
        UNIT_TEST(test_riscv_xor),
        UNIT_TEST(test_riscv_srl),
        UNIT_TEST(test_riscv_sra),
        UNIT_TEST(test_riscv_or),
        UNIT_TEST(test_riscv_lui),
        UNIT_TEST(test_riscv_sb),
        UNIT_TEST(test_riscv_lb),
        UNIT_TEST(test_riscv_auipc),
        UNIT_TEST(test_riscv_and),
        UNIT_TEST(test_riscv_jal),
        UNIT_TEST(test_riscv_jalr),
        UNIT_TEST(test_riscv_beq),
        UNIT_TEST(test_riscv_bne),
        UNIT_TEST(test_riscv_bltu),
        UNIT_TEST(test_riscv_bgeu),
        UNIT_TEST(test_riscv_blt),
        UNIT_TEST(test_riscv_bge),
        UNIT_TEST(test_riscv_mul),
        UNIT_TEST(test_riscv_div),
        UNIT_TEST(test_riscv_csr),
        UNIT_TEST(test_riscv_csri),
   };

    for (int i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
        printf("Run %s: ", tests[i].name);
        tests[i].test_func(riscv);
        printf("passed\n");
    }
}

#endif