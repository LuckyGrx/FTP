#include "threadpool.h"

void* threadpool_worker(void* p) {
	if (NULL == p)
		return NULL;
	ftp_threadpool_t* pool = (ftp_threadpool_t*)p;
	
	while (1) {
		//
		pthread_mutex_lock(&(pool->mutex));

		//printf("pid = %u\n", pthread_self());

		while (0 == pool->queuesize && !(pool->shutdown)) {
			// 先解锁,然后进入阻塞状态,信号来了之后,加上锁,最后返回
			pthread_cond_wait(&(pool->cond), &(pool->mutex));
		}

		// 立即停机模式、平滑停机且没有未完成任务则退出
		if (pool->shutdown == immediate_shutdown) {
			// pthread_cond_wait之后,又会加上互斥锁,所以这里必须释放互斥锁
			pthread_mutex_unlock(&(pool->mutex));
			break;
		} else if (pool->shutdown == graceful_shutdown && pool->queuesize == 0) {
			// pthread_cond_wait之后,又会加上互斥锁,所以这里必须释放互斥锁
			pthread_mutex_unlock(&(pool->mutex));
			break;
		}
	
		// 得到第一个task
		task_t* task = pool->head->next;

		if (NULL == task) {
			pthread_mutex_unlock(&(pool->mutex));
			continue;
		}

		// 存在task则取走并开锁
		pool->head->next = task->next;
		--(pool->queuesize);
		pthread_mutex_unlock(&(pool->mutex));


	    (*(task->func))(task->arg);
		free(task);
	}
	
	pthread_exit(NULL);
}

ftp_threadpool_t* threadpool_init(int threadnum){
	ftp_threadpool_t* pool;
	if ((pool = (ftp_threadpool_t*)calloc(1, sizeof(ftp_threadpool_t))) == NULL)
		goto err;
	if ((pool->threads = (pthread_t*)calloc(threadnum, sizeof(pthread_t))) == NULL)
		goto err;
	if ((pool->head = (task_t*)calloc(1, sizeof(task_t))) == NULL)  // 分配并初始化task头结点
		goto err;
	if (pthread_cond_init(&(pool->cond), NULL) != 0)
		goto err;
	if (pthread_mutex_init(&(pool->mutex), NULL) != 0)
		goto err;
	pool->threadnum = 0;
	pool->queuesize = 0;
	pool->shutdown = 0;

	int i;
	for(i = 0; i < threadnum; ++i) {
		if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, pool) != 0) {
			threadpool_destroy(pool, 0);
			return NULL;
		}
		pool->threadnum++;
	}
	
	return pool;

err:
	threadpool_free(pool);
	return NULL;
}


int threadpool_add(ftp_threadpool_t* pool, void (*func)(void*), void *arg) {
	if (NULL == pool || NULL == func)
		return -1;
	if (pthread_mutex_lock(&(pool->mutex)) != 0)
		return -1;

	task_t* task = (task_t*)calloc(1, sizeof(task_t));
	if (NULL == task)
		goto err;
	task->func = func;
	task->arg = arg;
	
	task->next = pool->head->next;
	pool->head->next = task;

	++(pool->queuesize);
	pthread_cond_signal(&(pool->cond));

err:
	pthread_mutex_unlock(&(pool->mutex));
	return 0;
}

int threadpool_free(ftp_threadpool_t* pool) {
	if (NULL == pool)
		return -1;

	free(pool->threads);

	task_t* task;
	while (pool->head->next) {
		task = pool->head->next;
		pool->head->next = pool->head->next->next;
		free(task);
	}
	free(pool->head);
	free(pool);
	return 0;
}

int threadpool_destroy(ftp_threadpool_t* pool, int shutdown_model) {
	if (NULL == pool)
		return -1;

	pool->shutdown = shutdown_model;

	pthread_cond_broadcast(&(pool->cond));

	for (int i = 0; i < pool->threadnum; ++i) {
		pthread_join(pool->threads[i], NULL); 
	}

	pthread_mutex_destroy(&(pool->mutex));
	pthread_cond_destroy(&(pool->cond));
	threadpool_free(pool);
	return 0;
}