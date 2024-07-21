#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mem.h"

static int mem_read (struct _riscv_device_t * dev, riscv_word_t addr, uint8_t * data, int size) {
    if ((dev->attr & RISCV_MEM_ATTR_READABLE) == 0) {
        fprintf(stderr, "write to readonly memory\n");
        return -1;
    }

    mem_t * mem = (mem_t *)dev;
    riscv_word_t offset = addr - dev->base;
    if (size == 4) {
        *(riscv_word_t *)data = *(riscv_word_t *)(mem->mem + offset);
    } else if (size == 2) {
        *(uint16_t *)data =  *(uint16_t *)(mem->mem + offset);
    } else if (size == 1) {
        *(uint8_t *)data =  *(uint8_t *)(mem->mem + offset);
    } else {
        memcpy(data, mem->mem + offset, size);
    }
    return 0;
}

static int mem_write (struct _riscv_device_t * dev,  riscv_word_t addr, uint8_t * data, int size) {
    if ((dev->attr & RISCV_MEM_ATTR_WRITABLE) == 0) {
        fprintf(stderr, "write to readonly memory\n");
        return -1;
    }

    mem_t * mem = (mem_t *)dev;
    riscv_word_t offset = addr - dev->base;
    if (size == 4) {
        *(riscv_word_t *)(mem->mem + offset) = *(riscv_word_t *)data;
    } else if (size == 2) {
        *(uint16_t *)(mem->mem + offset) = *(uint16_t *)data;
    } else if (size == 1) {
        *(uint8_t *)(mem->mem + offset) = *(uint8_t *)data;
    } else {
        memcpy(mem->mem + offset, data, size);
    }

    return 0;
}

mem_t * mem_create(const char * name, riscv_word_t base, riscv_word_t size, riscv_word_t attr) {
    mem_t * mem = (mem_t *)malloc(sizeof(mem_t));
    if (mem == NULL) {
        fprintf(stderr, "memory region malloc failed\n");
        return NULL;
    }

    mem->mem = malloc(size);
    if (mem->mem == NULL) {
        fprintf(stderr, "memory region malloc failed\n");
        free(mem);
        return NULL;
    }

    device_t * device = (device_t *)mem;
    device_init(device, name, attr, base, size);
    device->read = mem_read;
    device->write = mem_write;
    return mem;
}
