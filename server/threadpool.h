#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "head.h"

typedef struct task {
    void             (*func)(void*);    // 
    void*            arg;
    struct task*     next;
}task_t;

typedef struct threadpool {
	pthread_t* threads;     // 线程数组
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	task_t* head;           // 任务链表头节点
	int threadnum;
	int queuesize;          // 任务链表长度(不含头节点)
}threadpool_t;

threadpool_t* threadpool_init(int);
int threadpool_add(threadpool_t*, void (*func)(void*), void* arg);
void threadpool_get(threadpool_t*);
int threadpool_free(threadpool_t*);
int threadpool_destroy(threadpool_t*);

#endif