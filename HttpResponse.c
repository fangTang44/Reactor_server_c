#include "HttpResponse.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ResHeaderSize 16
// 初始化
struct HttpResponse* HttpResponseInit() {
	struct HttpResponse* response = (struct HttpResponse*)malloc(sizeof(struct HttpResponse));
	response->headerNum = 0;
	int size = sizeof(struct ResponseHeader) * ResHeaderSize;
	response->headers = (struct ResponseHeader*)malloc(size);
	response->statuseCode = UnKnown;
	// 初始化数组
	bzero(response->headers,size); 
	bzero(response->statusMsg, sizeof(response->statusMsg));
	bzero(response->fileName, sizeof(response->fileName));
	// 函数指针
	response->sendDataFunc = NULL;

	return response;
}

// 销毁
void httpResponseDestroy(struct HttpResponse* response) {
	if (response != NULL) {
		free(response->headers);
		free(response);
	}
}

// 添加响应头
void httpResponseAddHeader(struct HttpResponse* response, const char* key, const char* value) {
	if (response == NULL || key == NULL || value == NULL) {
		return;
	}
	strcpy(response->headers[response->headerNum].key, key);
	strcpy(response->headers[response->headerNum].valuse ,value);
	response->headerNum++;
}

// 组织http响应数据
void httpResponsePrepareMsg(struct HttpResponse* response, struct Buffer* sendBuf, int socket) {
	// 状态行
	char tmp[1024] = {0};
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", response->statuseCode, response->statusMsg);
	bufferAppendString(sendBuf, tmp);
	// 响应头
	for (int i = 0; i < response->headerNum; i++) {
		sprintf(tmp, "%s: %s\r\n", response->headers[i].key, response->headers[i].valuse);
		bufferAppendString(sendBuf, tmp);
	}
	// 空行
	bufferAppendString(sendBuf, "\r\n");
#ifndef MSG_SEND_AUTO
	bufferSendData(sendBuf, socket);
#endif

	// 回复的数据
	response->sendDataFunc(response->fileName, sendBuf, socket);
}