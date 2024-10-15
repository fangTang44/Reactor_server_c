#include "Channel.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// 初始化Channel
struct Channel* channelInit(int fd, int events, handleFunc readCallBack, handleFunc writeCallBack, handleFunc destroyFunc, void* arg) {
	struct Channel* channel = (struct Channel*)malloc(sizeof(struct Channel));
	assert(channel != NULL);
	channel->arg = arg;
	channel->fd = fd;
	channel->events = events;
	channel->readCallBack = readCallBack;
	channel->writeCallBack = writeCallBack;
	channel->destroyCollBack = destroyFunc;
	return channel;
}

// 修改fd的写事件(检测/不检测)
void writeEventEnable(struct Channel* channel, bool flag) {
	assert(channel != NULL);
	if (flag) {
		channel->events |= WriteEvent;	
	}
	else {
		channel->events = channel->events & ~WriteEvent;
	}
}

// 判断是否需要检测文件描述符的写事件
bool isWriteEventEnable(struct Channel* channel) {
	return channel->events & WriteEvent;
}