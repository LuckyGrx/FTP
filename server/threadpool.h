#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "head.h"

typedef struct task {
    void             (*func)(void*);    // 
    void*            arg;
    struct task*     next;
}task_t;

typedef struct ftp_threadpool {
	pthread_t* threads;     // 线程数组
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	task_t* head;           // 任务链表头节点
	int threadnum;
	int queuesize;          // 任务链表长度(不含头节点)

	int shutdown;           // 关机模式（平滑关闭、立即关闭)
}ftp_threadpool_t;

typedef enum {
	immediate_shutdown = 1,
	graceful_shutdown = 2
}ftp_threadpool_shutdown_t;

ftp_threadpool_t* threadpool_init(int);
int threadpool_add(ftp_threadpool_t*, void (*func)(void*), void* arg);
int threadpool_free(ftp_threadpool_t*);
int threadpool_destroy(ftp_threadpool_t* pool, int shutdown);

#endif