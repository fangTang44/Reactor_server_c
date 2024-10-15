#define _GNU_SOURCE
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <sys/socket.h>


// 初始化
struct Buffer* BufferInit(int size) {
	struct Buffer* buffer = (struct Buffer*)malloc(sizeof(struct Buffer));
	buffer->data = (char*)malloc(sizeof(char) * size);
	memset(buffer->data, 0, size);
	buffer->capacity = size;
	buffer->readPos = 0;
	buffer->writePos = 0;
	return buffer;
}

// 销毁内存
void bufferDestroy(struct Buffer* buf) {
	if (buf != NULL) {
		if (buf->data != NULL) {
			free(buf->data);
		}
		free(buf);
	}
}

// 扩容
void bufferExtendRoom(struct Buffer* buf, int size) {
	// 1. 内存够用 - 不需要扩容
	if (bufferWriteAbleSize(buf) >= size) {
		return;
	}
	// 2. 内存需要合并后才够用 - 不需要扩容
	// 剩余的可写的内存 + 已读的内存 > size
	else if (buf->readPos + bufferWriteAbleSize(buf) >= size) {
		// 得到未读的内存大小
		int readAble = bufferReadAbleSize(buf);
		// 移动内存
		memcpy(buf->data, buf->data + buf->readPos, readAble);
		// 更新位置
		buf->readPos = 0;
		buf->writePos = readAble;
	}
	// 3. 内存不够用 - 扩容
	else {
		void* temp = realloc(buf->data, buf->capacity + size);
		if (temp == NULL) {
			return;
		}
		memset(temp + buf->capacity, 0, size);
		// 更新
		buf->data = temp;
		buf->capacity += size;
	}
	return;
}

// 得到剩余可写内存容量
int bufferWriteAbleSize(struct Buffer* buf) {
	if (buf == NULL) {
		return -1;
	}
	return buf->capacity - buf->writePos;
}

// 得到剩余可读内存容量
int bufferReadAbleSize(struct Buffer* buf) {
	if (buf == NULL) {
		return -1;
	}
	return buf->writePos - buf->readPos;
}

// 写内存 1. 直接写， 2. 套接字接收数据
int bufferAppendData(struct Buffer* buf, const char* data, int size) {
	if (buf == NULL || data == NULL || data <= 0) {
		return -1;
	}
	// 扩容
	bufferExtendRoom(buf, size);
	// 数据拷贝
	memcpy(buf->data + buf->writePos, data, size);
	buf->writePos += size;
	return 0;
}

int bufferAppendString(struct Buffer* buf, const char* data) {
	int size = strlen(data);
	int ret = bufferAppendData(buf, data, size);
	return ret;
}

int bufferSocketRead(struct Buffer* buf, int fd) {
	// read/recv/readv
	struct iovec vec[2];
	// 初始化
	int writeAble = bufferWriteAbleSize(buf);
	vec[0].iov_base = buf->data + buf->readPos;
	vec[0].iov_len = writeAble;
	char* tempbuf = (char*)malloc(40960);
	vec[1].iov_base = tempbuf;
	vec[1].iov_len = 40960;
	int result = readv(fd, vec, 2);
	if (result == -1) {
		return -1;
	}
	// buf中的内存未用尽
	else if (result <= writeAble) {
		buf->writePos += result;
	}
	else {
		buf->writePos = buf->capacity;
		bufferAppendData(buf, tempbuf, result - writeAble);
	}
	free(tempbuf);
	return result;
}

// 根据/r/n取出一行,找到其在数据块中的位置,返回该位置
char* bufferFindCRLF(struct Buffer* buffer) {
	// strstr --> 大字符串中匹配子字符串(遇到\n结束)
	// memmem --> 从大数据块中匹配子数据块(需要制定数据块大小)
  char* ptr =	memmem(buffer->data + buffer->readPos, bufferReadAbleSize(buffer), "\r\n", 2);
	return ptr;
}

// 发送数据
int bufferSendData(struct Buffer* buffer, int socket) {
	// 判断有无未读数据
	int readable = bufferReadAbleSize(buffer);
	if (readable > 0) {
		int count = send(socket, buffer->data + buffer->readPos, readable, MSG_NOSIGNAL);
		if (count > 0) {
			buffer->readPos += count;
			usleep(1);
		}
		return count;
	}
	return 0;
}