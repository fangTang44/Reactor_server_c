#include "WorkerThread.h"
#include <stdio.h>

// 初始化
int workerThreadInit(struct WorkerThread* thread, int index) {
	thread->evLoop = NULL;
	thread->threadID = 0;
	sprintf(thread->name, "SubThread-%d", index);
	pthread_mutex_init(&thread->mutex, NULL);
	pthread_cond_init(&thread->cond, NULL);
	return 0;
}

// 子线程的任务函数
void* subThreadRunning(void* arg) {
	struct WorkerThread* thread = (struct WorkerThread*)arg;
	pthread_mutex_lock(&thread->mutex);
	thread->evLoop = eventLoopInit(thread->name);
	pthread_mutex_unlock(&thread->mutex);
	pthread_cond_signal(&thread->cond);
	eventLoopRun(thread->evLoop);
	return NULL;
}

// 启动子线程
void workerThreadRun(struct WorkerThread* thread) {
	// 创建子线程
	pthread_create(&thread->threadID, NULL, subThreadRunning, thread);
	// 使用条件变量对主线程进行阻塞  等待子线程创建成功
	pthread_mutex_lock(&thread->mutex);
	while (thread->evLoop == NULL) {
		pthread_cond_wait(&thread->cond, &thread->mutex);
	}
	pthread_mutex_unlock(&thread->mutex);
}