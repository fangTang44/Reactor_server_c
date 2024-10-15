#include "ThreadPool.h"
#include "EventLoop.h"
#include <assert.h>
#include <stdlib.h>

// 初始化
struct ThreadPool* threadPoolInit(struct EventLoop* mainLoop, int count) {
	struct ThreadPool* pool = (struct ThreadPool*)malloc(sizeof(struct ThreadPool));
	assert(pool != NULL);
	pool->index = 0;
	pool->isStart = false;
	pool->mainLoop = mainLoop;
	pool->threadNum = count;
	pool->workerThreads = (struct WorkerThread*)malloc(sizeof(struct WorkerThread) * count);
	return pool;
}

// 启动线程池
void threadPoolRun(struct ThreadPool* pool) {
	assert(pool != NULL && !pool->isStart);
	if (pool->mainLoop->threadID != pthread_self()) {
		exit(0);
	}
	pool->isStart = true;
	if (pool->threadNum) {
		for (int i = 0; i < pool->threadNum; i++) {
			workerThreadInit(&pool->workerThreads[i], i);
			workerThreadRun(&pool->workerThreads[i]);
		}
	}
}

// 取出某个子线程的反应堆实例
struct EventLoop* takeWorkerEventLoop(struct ThreadPool* pool) {
	assert(pool->isStart);	
	if (pool->mainLoop->threadID != pthread_self()) {
		exit(0);
	}
	// 从线程池中找一个子线程，取出反应堆实例
	struct EventLoop* evLoop = pool->mainLoop;
	if (pool->threadNum > 0) {
		evLoop = pool->workerThreads[pool->index].evLoop;
		pool->index = ++pool->index % pool->threadNum;
	}
	return evLoop;
}