#include "Dispatcher.h"
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define Max 1024
struct SelectData {
	fd_set readSet;
	fd_set writeSet;
};
static void* selectInit();
static int selectAdd(struct Channel* channel, struct EventLoop* evLoop);
static int selectRemove(struct Channel* channel, struct EventLoop* evLoop);
static int selectModify(struct Channel* channel, struct EventLoop* evLoop);
static int selectDispatch(struct EventLoop* evLoop, int timeOut); // s
static int selectClear(struct EventLoop* evLoop);
static int setFdSet(struct Channel* channel, struct SelectData* data);
static int clearFdSet(struct Channel* channel, struct SelectData* data);

struct Dispatcher SelectDispatcher = {
	selectInit,
	selectAdd,
	selectRemove,
	selectModify,
	selectDispatch,
	selectClear
};


static void* selectInit() {
	struct SelectData*data = (struct SelectData*)malloc(sizeof(struct SelectData));
	assert(data != NULL);
	FD_ZERO(&data->readSet);
	FD_ZERO(&data->writeSet);
	return data;
}

static int setFdSet(struct Channel* channel, struct SelectData* data) {
	if (channel->events & ReadEvent) {
		FD_SET(channel->fd, &data->readSet);
	}
	if (channel->events & WriteEvent) {
		FD_SET(channel->fd, &data->writeSet);
	}
	return 0;
}
static int clearFdSet(struct Channel* channel, struct SelectData* data) {
	if (channel->events & ReadEvent) {
		FD_CLR(channel->fd, &data->readSet);
	}
	if (channel->events & WriteEvent) {
		FD_CLR(channel->fd, &data->writeSet);
	}
	return 0;
}

static int selectAdd(struct Channel* channel, struct EventLoop* evLoop) {
	struct SelectData* data = (struct SelectData*)evLoop->disPatcherData;
	if (channel->fd >= 1024) {
		return -1;
	}
	setFdSet(channel, data);
	return 0;
}

static int selectRemove(struct Channel* channel, struct EventLoop* evLoop) {
	struct SelectData* data = (struct SelectData*)evLoop->disPatcherData;
	if (channel->fd >= 1024) {
		return -1;
	}
	clearFdSet(channel, data);
	// 通过channel释放对应的TcpConnection资源
	channel->destroyCollBack(channel->arg);
	return 0;
}

static int selectModify(struct Channel* channel, struct EventLoop* evLoop) {
	struct SelectData* data = (struct SelectData*)evLoop->disPatcherData;
	if (channel->fd >= 1024) {
		return -1;
	}
	setFdSet(channel, data);
	clearFdSet(channel, data);
	return 0;
}

static int selectDispatch(struct EventLoop* evLoop, int timeOut) { // s
	struct SelectData* data = (struct SelectData*)evLoop->disPatcherData;
	struct timeval val;
	val.tv_sec = timeOut;
	val.tv_usec = 0;
	fd_set rdtemp = data->readSet;
	fd_set wrtemp = data->writeSet;
	int count = select(Max, &rdtemp, &wrtemp, NULL, &val);
	if (count == -1) {
		perror("select");
		exit(0);
	}
	for (int i = 0; i < Max; i++) {
		if (FD_ISSET(i, &rdtemp)) {
			eventActivate(evLoop, i, ReadEvent);
		}
		if (FD_ISSET(i, &wrtemp)) {
			eventActivate(evLoop, i, WriteEvent);
		}
	}
	return 0;
}

static int selectClear(struct EventLoop* evLoop) {
	struct SelectData* data = (struct SelectData*)evLoop->disPatcherData;
	assert(data != NULL);
	free(data);
	return 0;
}