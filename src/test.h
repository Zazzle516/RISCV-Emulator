#ifndef TEST_H
#define TEST_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct _test_unit_t {
    const char* name;
    void (*func) (void*);
}test_unit_t;

#define UNIT_TEST(f)     {#f, f}

static void run_test_group(const test_unit_t* test_group, int size, void* param) {
    for (int i = 0; i < size; i++) {
        const test_unit_t* unit = test_group + i;
        printf("[Run Test %s]\n", unit->name);
        unit->func(param);
        printf("[     OK]\n");
    }
}

#define assert_int_equal(a, b)      \
    if (a != b) {   \
        fprintf(stderr, "test failed: %d != b(%s: %d)\n", a, b, __FILE__, __LINE__);    \
        exit(-1);       \
    }

#include <stdio.h>
#include <SDL.h>
#include "plat/plat.h"

static void test_riscv_0(void* state) {
}

static void test_riscv_1(void* state) {
}

void test_thread_entry (void * param) {

    // 初始化 SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        exit(-1);
    }

    // 创建一个窗口
    SDL_Window* window = SDL_CreateWindow("test ok", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        exit(-1);
    }

    // 创建渲染器
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(-1);
    }

    SDL_Event event;
    int quit = 0;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = 1;
            }
        }

        // 渲染
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    // 清理
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void test_env(void) {
    static const test_unit_t tests[] = {
        UNIT_TEST(test_riscv_0),
        UNIT_TEST(test_riscv_1),
    };

    run_test_group(tests, sizeof(tests) / sizeof(test_unit_t), NULL);
    thread_create(test_thread_entry, NULL);     
    for (;;) {
        thread_msleep(1000);
    }
}

#endif // !TEST_H
