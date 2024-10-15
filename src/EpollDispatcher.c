#include "Dispatcher.h"
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#define Max 520
struct EpollData {
	int epfd;
	struct epoll_event* events;
};
static void* epollInit();
static int epollAdd(struct Channel* channel, struct EventLoop* evLoop);
static int epollRemove(struct Channel* channel, struct EventLoop* evLoop);
static int epollModify(struct Channel* channel, struct EventLoop* evLoop);
static int epollDispatch(struct EventLoop* evLoop, int timeOut); // s
static int epollClear(struct EventLoop* evLoop);
static int epollCtl(struct Channel* channel, struct EventLoop* evLoop, int op);

struct Dispatcher EpollDispatcher = {
	epollInit,
	epollAdd,
	epollRemove,
	epollModify,
	epollDispatch,
	epollClear
};


static void* epollInit() {
	struct EpollData*data = (struct EpollData*)malloc(sizeof(struct EpollData));
	assert(data != NULL);
	data->epfd = epoll_create(10);
	if (data->epfd == -1) {
		perror("epoll_create");
		exit(0);
	}
	data->events = (struct epoll_event*)calloc(Max, sizeof(struct epoll_event));
	return data;
}

static int epollCtl(struct Channel* channel, struct EventLoop* evLoop, int op) {
	struct EpollData* data = (struct EpollData*)evLoop->disPatcherData;
	struct epoll_event ev;
	ev.data.fd = channel->fd;
	int events = 0;
	if (channel->events & ReadEvent) {
		events |= EPOLLIN;
	}
	if (channel->events & WriteEvent) {
		events |= EPOLLOUT;
	}
	ev.events = events;
	int ret = epoll_ctl(data->epfd, op, channel->fd, &ev);
	return ret;
}

static int epollAdd(struct Channel* channel, struct EventLoop* evLoop) {
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_ADD);
	if (ret == -1) {
		perror("epoll add error");
		exit(0);
	}
	return ret;
}

static int epollRemove(struct Channel* channel, struct EventLoop* evLoop) {
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_DEL);
	if (ret == -1) {
		perror("epoll del error");
		exit(0);
	}
	// 通过channel释放对应的TcpConnection资源
	channel->destroyCollBack(channel->arg);
	return ret;
}

static int epollModify(struct Channel* channel, struct EventLoop* evLoop) {
	int ret = epollCtl(channel, evLoop, EPOLL_CTL_MOD);
	if (ret == -1) {
		perror("epoll mod error");
		exit(0);
	}
	return ret;
}

static int epollDispatch(struct EventLoop* evLoop, int timeOut) { // s
	struct EpollData*data = (struct EpollData*)evLoop->disPatcherData;
	assert(data != NULL);
	int count = epoll_wait(data->epfd, data->events, Max, timeOut * 1000);
	for (int i = 0; i < count; i++) {
		int events = data->events[i].events;
		int fd = data->events[i].data.fd;
		// 若对方断开连接 对段断开连接继续发送数据
		if (events & EPOLLERR || events & EPOLLHUP) {
			// 删除fd
			// epollRemove(Channel, evLoop);
			continue;
		}
		if (events & EPOLLIN) {
			eventActivate(evLoop, fd, ReadEvent);
		}
		if (events & EPOLLOUT) {
			eventActivate(evLoop, fd, WriteEvent);
		}
	}
	return 0;
}

static int epollClear(struct EventLoop* evLoop) {
	struct EpollData*data = (struct EpollData*)malloc(sizeof(struct EpollData));
	assert(evLoop != NULL);
	free(data->events);
	close(data->epfd);
	free(data);
	return 0;
}