#include "threadpool.h"

void* threadpool_worker(void* p) {
	threadpool_t* pool = (threadpool_t*)p;
	while (1) {
		threadpool_get(pool);
	}
}

threadpool_t* threadpool_init(int threadnum){
	threadpool_t* pool;
	if ((pool = (threadpool_t*)calloc(1, sizeof(threadpool_t))) == NULL)
		goto err;
	if ((pool->threads = (pthread_t*)calloc(threadnum, sizeof(pthread_t))) == NULL)
		goto err;
	if ((pool->head = (task_t*)calloc(1, sizeof(task_t))) == NULL)  // 分配并初始化task头结点
		goto err;
	if (pthread_cond_init(&(pool->cond), NULL) != 0)
		goto err;
	if (pthread_mutex_init(&(pool->mutex), NULL) != 0)
		goto err;
	pool->threadnum = threadnum;
	pool->queuesize = 0;

	int i;
	for(i = 0; i < pool->threadnum; ++i) {
		if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, pool) != 0) {
			threadpool_destroy(pool);
			return NULL;
		}
	}
	return pool;

err:
	if (pool)
		threadpool_free(pool);
	return NULL;
}


int threadpool_add(threadpool_t* pool, void (*func)(void*), void *arg) {
	if (NULL == pool)
		return -1;
	pthread_mutex_lock(&(pool->mutex));

	task_t* task = (task_t*)calloc(1, sizeof(task_t));
	task->func = func;
	task->arg = arg;
	
	task->next = pool->head->next;
	pool->head->next = task;

	++(pool->queuesize);
	pthread_cond_signal(&(pool->cond));

	pthread_mutex_unlock(&(pool->mutex));
	return 0;
}

void threadpool_get(threadpool_t* pool) {
	if (NULL == pool)
		return ;

	pthread_mutex_lock(&(pool->mutex));
	while (0 == pool->queuesize)
		pthread_cond_wait(&(pool->cond), &(pool->mutex));
	task_t* task = pool->head->next;
	--(pool->queuesize);
    (*(task->func))(task->arg);
	pthread_mutex_unlock(&(pool->mutex));
}

int threadpool_free(threadpool_t* pool) {
	if (NULL == pool)
		return -1;
	if (pool->threads)
		free(pool->threads);

	task_t* task;
	while (pool->head->next) {
		task = pool->head->next;
		pool->head->next = pool->head->next->next;
		free(task);
	}
	free(pool->head);
	return 0;
}

int threadpool_destroy(threadpool_t* pool) {
	if (NULL == pool)
		return -1;
	int i;
	for (i = 0; i < pool->threadnum; ++i) {
		if (pthread_join(pool->threads[i], NULL) != 0)
			return -1;
	}
	return 0;
}