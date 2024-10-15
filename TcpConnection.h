#pragma oncde 
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include "HttpResponse.h"

// #define MSG_SEND_AUTO
struct TcpConnection
{
	struct EventLoop* evLoop;
	struct Channel* channel;
	struct Buffer* readBuf;
	struct Buffer* writeBuf;
	char name[32];
	// http协议
	struct HttpRequest* request;
	struct HttpResponse* response;
};

// 初始化	
struct TcpConnection* tcpConnectionInit(int fd, struct EventLoop* evLoop);
//
int tcpConnectionDestroy(void* arg);
