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
	void (*entry)(void*);
	void* param;
}thread_t;

typedef SOCKET socket_t;

static DWORD WINAPI thread_wrapper(LPVOID lpParam) {
	thread_t * thread = (thread_t *)lpParam;
	thread->entry(thread->param);
	free(thread);
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

static void plat_init(void) {
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	int rc = WSAStartup(sockVersion, &wsaData);
	assert(rc == 0);
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
