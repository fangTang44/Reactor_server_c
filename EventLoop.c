#include "EventLoop.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

// 初始化
struct EventLoop* eventLoopInit() {
	eventLoopInitEx(NULL);
}

// 写数据
void taskWakeup(struct EventLoop* evLoop) {
	const char* msg = "start work";
	write(evLoop->socketPair[0], msg, strlen(msg));
}

// 读数据
int readLocalMessage(void* arg) {
	struct EventLoop* evLoop = (struct EventLoop*)arg;
	char buf[256];
	read(evLoop->socketPair[1], buf, sizeof(buf));
	return 0;
}

struct EventLoop* eventLoopInitEx(const char* threadName) {
	struct EventLoop* evLoop = (struct EventLoop*)malloc(sizeof(struct EventLoop));
	assert(evLoop != NULL);
	evLoop->isQuit = false;
	evLoop->threadID = pthread_self();
	pthread_mutex_init(&evLoop->mutex, NULL);
	strcpy(evLoop->threadName, threadName == NULL ? "MainThread" : threadName);
	// evLoop->dispatcher = &EpollDispatcher;
	// evLoop->dispatcher = &PollDispatcher;
	evLoop->dispatcher = &SelectDispatcher;
	evLoop->disPatcherData = evLoop->dispatcher->init();
	// 链表
	evLoop->head = evLoop->tail = NULL;
	// map
	evLoop->channelMap = channelMapInit(128);
	// sockerPait
	int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, evLoop->socketPair);
	if (ret == -1) {
		perror("socketpair");
		exit(0);
	}
	// 指定规则： evLoop->socketPair[0] 发送数据，evLoop->socketPiar[1]接收数据
	struct Channel* channel = channelInit(evLoop->socketPair[1], 
	ReadEvent, readLocalMessage, NULL, NULL, evLoop);
	// channel添加到任务队列
	eventLoopAddTask(evLoop, channel, ADD);

	return evLoop;
}

// 启动反应堆模型
int eventLoopRun(struct EventLoop* evLoop) {
	assert(evLoop != NULL);
	// 比较线程ID是否正常
	if (evLoop->threadID != pthread_self()) {
		return -1;
	}
	// 取出事件分发器和检测模型
	struct Dispatcher* dispatcher = evLoop->dispatcher;
	// 循环进行事件处理
	while (!evLoop->isQuit) {
		dispatcher->dispatch(evLoop, 2);
		eventLoopProcessTask(evLoop);
	}
	return 0;
}

// 处理被激活的文件描述符
int eventActivate(struct EventLoop* evLoop, int fd, int event) {
	if (fd < 0 || evLoop == NULL) {
		return -1;
	}
	// 取出channel
	struct Channel* channel = evLoop->channelMap->list[fd];
	assert(channel->fd == fd);
	// 判断读写事件
	if (event & ReadEvent && channel->readCallBack) {
		channel->readCallBack(channel->arg);
	}
	if (event & WriteEvent && channel->writeCallBack) {
		channel->writeCallBack(channel->arg);
	}
	return 0;
}

// 添加任务到任务队列
int eventLoopAddTask(struct EventLoop* evLoop, struct Channel* channel, int type) {
	// 加锁,保护共享资源
	pthread_mutex_lock(&evLoop->mutex);
	// 创建新节点
	struct ChannelElement* node = (struct ChannelElement*)malloc(sizeof(struct ChannelElement));
	assert(node != NULL);
	node->channel = channel;
	node->type = type;
	node->next = NULL;
	// 将节点添加到任务队列中
	// 链表为空
	if (evLoop->head == NULL) {
		evLoop->head = evLoop->tail = node;
	}
	else {
		evLoop->tail->next = node; // add
		evLoop->tail = node;			 // move
	}
	pthread_mutex_unlock(&evLoop->mutex);
	// 处理节点
	if (evLoop->threadID == pthread_self()) {
		// 子线程
		eventLoopProcessTask(evLoop);
	}
	else {
		// 主线程 告诉子线程处理任务队列的任务 
		// 通过自己添加一个文件描述符，对自己的文件描述符进行读写使得子线程解除阻塞
		taskWakeup(evLoop);
	}
	return 0;
}

// 处理任务队列中的任务
int eventLoopProcessTask(struct EventLoop* evLoop) {
	pthread_mutex_lock(&evLoop->mutex);
	struct ChannelElement* head = evLoop->head;
	while (head != NULL) {
		struct Channel* channel = head->channel;
		if (head->type == ADD) {
			// 添加
			eventLoopAdd(evLoop, channel);
		}
		if (head->type == DELETE) {
			// 删除
			eventLoopRemove(evLoop, channel);
		}
		if (head->type == MODIFY) {
			// 修改
			eventLoopModify(evLoop, channel);
		}
		struct ChannelElement* tmp = head;
		head = head->next;
		free(tmp);
	}
	evLoop->head = evLoop->tail = NULL;
	pthread_mutex_unlock(&evLoop->mutex);
	return 0;
}

// 处理dispatcher中的节点
int eventLoopAdd(struct EventLoop* evLoop, struct Channel* channel) {
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size) {
		// 没有足够的空间存储键值对 扩容
		if (!makeMapRoom(channelMap, fd, sizeof(struct Channel*))) {
			return -1;
		}
	}
	// 找到fd对应的数组元素位置，并存储
	if (channelMap->list[fd] == NULL) {
		channelMap->list[fd] = channel;
		evLoop->dispatcher->add(channel, evLoop);
	}
	return 0;
}

int eventLoopRemove(struct EventLoop* evLoop, struct Channel* channel) {
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size) {
		return -1;
	}
	int ret = evLoop->dispatcher->remove(channel, evLoop);
	return ret;
}

int eventLoopModify(struct EventLoop* evLoop, struct Channel* channel) {
	int fd = channel->fd;
	struct ChannelMap* channelMap = evLoop->channelMap;
	if (fd >= channelMap->size || channelMap->list[fd] == NULL) {
		return -1;
	}
	int ret = evLoop->dispatcher->modify(channel, evLoop);
	return ret;
}

// 释放channel
int destroyChannel(struct EventLoop* evLoop, struct Channel* channel) {
	// 删除channel 和 fd的关系
	evLoop->channelMap->list[channel->fd] = NULL;
	// 关闭fd
	close(channel->fd);
	// 释放内存
	free(channel);
	return 0;
}