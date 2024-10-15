#pragma once
#include <stdbool.h>
// 定义函数指针
typedef int(*handleFunc)(void*);
// 定义文件描述符的读写事件(events)
enum FDEvent {
	TimeOut = 0X01,
	ReadEvent = 0X02,
	WriteEvent = 0X04
};

// 文件描述符以及事件
struct Channel {
	// 文件描述符
	int fd;
	// 事件
	int events;	// 按位处理
	// 回调函数
	handleFunc readCallBack;
	handleFunc writeCallBack;
	handleFunc destroyCollBack;
	// 回调函数的参数
	void* arg;
};

// 初始化Channel
struct Channel* channelInit(int fd, int events, handleFunc readCallBack, handleFunc writeCallBack, handleFunc handleFunc, void* arg);
// 修改fd的写事件(检测/不检测)
void writeEventEnable(struct Channel* channel, bool flag);
// 判断是否需要检测文件描述符的写事件
bool isWriteEventEnable(struct Channel* channel);