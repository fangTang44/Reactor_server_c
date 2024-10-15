#pragma once

struct Buffer {
	char* data;
	int capacity;
	int readPos;
	int writePos;
};

// 初始化
struct Buffer* BufferInit(int size);
// 销毁内存
void bufferDestroy(struct Buffer* buf);
// 扩容
void bufferExtendRoom(struct Buffer* buf, int Size);
// 得到剩余可写内存容量
int bufferWriteAbleSize(struct Buffer* buf);
// 得到剩余可读内存容量
int bufferReadAbleSize(struct Buffer* buf);
// 写内存 1. 直接写， 2. 套接字接收数据
int bufferAppendData(struct Buffer* buf, const char* data, int size);
int bufferAppendString(struct Buffer* buf, const char* data);
int bufferSocketRead(struct Buffer* buf, int fd);
// 根据/r/n取出一行,找到其在数据块中的位置,返回该位置
char* bufferFindCRLF(struct Buffer* buffer);
// 发送数据
int bufferSendData(struct Buffer* buffer, int socket);