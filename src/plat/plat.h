#ifndef PLAT_H
#define PLAT_H

#ifdef _WIN32

#include <Windows.h>
#include <winsock.h>
#include <stdlib.h>
#include <assert.h>
#include <io.h>
#include <fcntl.h>

#pragma comment(lib,"ws2_32.lib")  

typedef struct _thread_t {
	void (*entry)(void*);		// 接收任意单参数的并且没有返回值的函数指针
	void* param;				// 传递具体的任意类型参数
}thread_t;

typedef SOCKET socket_t;

// DWORD: 32 位的无符号整数类型
// WINAPI: 函数调用约定		__stdcall	从右到左读取参数
// LPVOID: 任意类型 类似于 void*
// lpParam: long pointer param	长指针参数	命名习惯
static DWORD WINAPI thread_wrapper(LPVOID lpParam) {
	thread_t * thread = (thread_t *)lpParam;
	thread->entry(thread->param);		// 执行传入的函数指针
	free(thread);						// 释放
	return 0;
}

static void thread_create(void (*entry)(void *), void * param) {
	thread_t* thread = (thread_t*)malloc(sizeof(thread_t));
	thread->entry = entry;
	thread->param = param;
	HANDLE handle = CreateThread(NULL, 0, thread_wrapper, thread, 0, NULL);
}


static void thread_msleep(int ms) {
	Sleep(ms);
}

// 使用 Winsock 之前的固定流程
static void plat_init(void) {
	// WORD: 16 位无符号整数类型
	// MAKEWORD(a, b): 把两个 1B 的数合并为一个 2B 的数 在函数中表示 Winsock 版本 2.2
	WORD sockVersion = MAKEWORD(2, 2);

	// 定义 WSADATA 结构体对象
	WSADATA wsaData;

	// 初始化
	// mVersionRequested: sockVersion
	int rc = WSAStartup(sockVersion, &wsaData);

	// 检查初始化结果
	assert(rc == 0);

	// 在完成网络通信后需要调用 WSACleanup() 清理
}

#else
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <netinet/tcp.h>

#define SOCKET_ERROR 	-1
#define INVALID_SOCKET	-1

typedef struct _thread_t {
	void (*entry)(void*);
	void* param;
}thread_t;

typedef int socket_t;

static void * thread_wrapper(void * param) {
	thread_t * thread = (thread_t *)param;
	thread->entry(thread->param);
	free(thread);
	return NULL;
}

static void thread_create(void (*entry)(void *), void * param) {
	thread_t* thread = (thread_t*)malloc(sizeof(thread_t));
	thread->entry = entry;
	thread->param = param;

	pthread_t s;
	int rc = pthread_create(&s, NULL, thread_wrapper, thread);
	assert(rc == 0);
}


static void thread_msleep(int ms) {
	usleep(ms * 1000);
}

static void plat_init(void) {
}
#endif

#endif // !PLAT_H
