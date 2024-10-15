#pragma once
#include "Channel.h"
#include "EventLoop.h"
struct EventLoop;
// 事件分发模型(io复用)
struct Dispatcher {
	// 初始化 初始化epoll,poll,select的数据块
	void* (*init)();
	// 添加
	int (*add)(struct Channel* channel, struct EventLoop* evLoop);
	// 删除
	int (*remove)(struct Channel* channel, struct EventLoop* evLoop);
	// 修改
	int (*modify)(struct Channel* channel, struct EventLoop* evLoop);
	// 事件检测
	int (*dispatch)(struct EventLoop* evLoop, int timeOut); // s
	// 清除数据(关闭fd活着释放内存)	
	int (*clear)(struct EventLoop* evLoop);
};