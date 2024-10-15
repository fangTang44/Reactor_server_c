#include "Dispatcher.h"
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define Max 1024
struct PollData {
	int maxFd;
	struct pollfd fds[Max];
};
static void* pollInit();
static int pollAdd(struct Channel* channel, struct EventLoop* evLoop);
static int pollRemove(struct Channel* channel, struct EventLoop* evLoop);
static int pollModify(struct Channel* channel, struct EventLoop* evLoop);
static int pollDispatch(struct EventLoop* evLoop, int timeOut); // s
static int pollClear(struct EventLoop* evLoop);

struct Dispatcher PollDispatcher = {
	pollInit,
	pollAdd,
	pollRemove,
	pollModify,
	pollDispatch,
	pollClear
};


static void* pollInit() {
	struct PollData*data = (struct PollData*)malloc(sizeof(struct PollData));
	assert(data != NULL);
	data->maxFd = 0;
	for (int i = 0; i < Max; i++) {
		data->fds[i].fd = -1;
		data->fds[i].events = 0;
		data->fds[i].revents = 0;
	}
	return data;
}

static int pollAdd(struct Channel* channel, struct EventLoop* evLoop) {
	struct PollData* data = (struct PollData*)evLoop->disPatcherData;
	int events = 0;
	if (channel->events & ReadEvent) {
		events |= POLLIN;
	}
	if (channel->events & WriteEvent) {
		events |= POLLOUT;
	}
	int i = 0;
	for (i = 0; i < Max; i++) {
		if (data->fds[i].fd == -1) {
			data->fds[i].fd = channel->fd;
			data->fds[i].events = events;
			data->maxFd = i > data->maxFd ? i : data->maxFd;
			break;
		}
	}
	if (i >= Max) {
		return -1;
	}
	return 0;
}

static int pollRemove(struct Channel* channel, struct EventLoop* evLoop) {
	struct PollData* data = (struct PollData*)evLoop->disPatcherData;
	int i = 0;
	for (i = 0; i < Max; i++) {
		if (data->fds[i].fd == channel->fd) {
			data->fds[i].fd = -1;
			data->fds[i].events = 0;
			data->fds[i].revents = 0;
			break;
		}
	}
	// 通过channel释放对应的TcpConnection资源
	channel->destroyCollBack(channel->arg);
	if (i >= Max) {
		return -1;
	}
	return 0;
}

static int pollModify(struct Channel* channel, struct EventLoop* evLoop) {
	struct PollData* data = (struct PollData*)evLoop->disPatcherData;
	int events = 0;
	if (channel->events & ReadEvent) {
		events |= POLLIN;
	}
	if (channel->events & WriteEvent) {
		events |= POLLOUT;
	}
	int i = 0;
	for (i = 0; i < Max; i++) {
		if (data->fds[i].fd == channel->fd) {
			data->fds[i].events = events;
			break;
		}
	}
	if (i >= Max) {
		return -1;
	}
	return 0;
}

static int pollDispatch(struct EventLoop* evLoop, int timeOut) { // s
	struct PollData* data = (struct PollData*)evLoop->disPatcherData;
	int count = poll(data->fds, data->maxFd + 1, timeOut * 1000);
	if (count == -1) {
		perror("poll");
		exit(0);
	}
	for (int i = 0; i <= data->maxFd; i++) {
		if (data->fds[i].fd == -1) {
			continue;
		}
		if (data->fds[i].revents & POLLIN) {
			eventActivate(evLoop, data->fds[i].fd, ReadEvent);
		}
		if (data->fds[i].revents & POLLOUT) {
			eventActivate(evLoop, data->fds[i].fd, WriteEvent);
		}
	}
	return 0;
}

static int pollClear(struct EventLoop* evLoop) {
	struct PollData* data = (struct PollData*)evLoop->disPatcherData;
	assert(data != NULL);
	free(data);
	return 0;
}