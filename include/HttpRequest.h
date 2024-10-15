#pragma once
#include "Buffer.h"
#include <stdbool.h>
#include "HttpResponse.h"


// 请求头键值对
struct RequestHeader {
	char* key;
	char* value;
};

// 当前解析状态
enum HttpRequestState {
	ParseReqLine,
	ParseReqHeaders,
	ParseReqBody,
	ParseReqDone
};

// http请求结构体
struct HttpRequest {
	char* method;	
	char* url;	
	char* version;	
	struct RequestHeader* reqHeaders;
	int reqHeadersNum;
	enum HttpRequestState curState;
};

// 初始化
struct HttpRequest* httpRequestInit();
// 重置
void httpRequestReset(struct HttpRequest* req);
void httpRequestResetEx(struct HttpRequest* req);
void httpRequestDestroy(struct HttpRequest* req);
// 获取处理状态
enum HttpRequestState httpRequestState(struct HttpRequest req);
// 添加请求头
void httpRequestAddHeader(struct HttpRequest* req, const char* key, const char* val);
// 根据key得到val
char* httpRequestGetHeader(struct HttpRequest* req, const char* key);
// 解析请求行
bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* readBuf);
// 解析请求头
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf);
// 解析http请求协议
bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf, 
struct HttpResponse* response, struct Buffer* sendBuf, int socket);
// 处理http请求协议
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* reponse);
// 解码字符串
void decodeMsg(char* to, char* from);
// 获取文件类型对应的type
const char* getFileType(const char* name);
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd);
void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd);