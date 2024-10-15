#include "TcpServer.h"
#include <arpa/inet.h>
#include "TcpConnection.h"
#include <stdio.h>
#include <stdlib.h>
#include "Log.h"

// 初始化
struct TcpServer* tcpServerInit(unsigned short port, int threadNum) {
	struct TcpServer* tcp = (struct TcpServer*)malloc(sizeof(struct TcpServer));
	tcp->listener = listenerInit(port);
	tcp->mainLoop = eventLoopInit();
	tcp->threadNum = threadNum;
	tcp->threadPool = threadPoolInit(tcp->mainLoop, tcp->threadNum);
	return tcp;
}

// 初始化监听
struct Listener* listenerInit(unsigned short port) {
	struct  Listener* listener = (struct Listener*)malloc(sizeof(struct Listener));
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("socket");
		exit(0);
	}
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1) {
		perror("setsockopt");
		exit(0);
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1) {
		perror("bind");
		exit(0);
	}
	ret = listen(lfd, 128);
	if (ret == -1) {
		perror("listen");
		exit(0);
	}	
	listener->lfd = lfd;
	listener->port = port;
	return listener;
}

int acceptConnection(void* arg) {
	struct TcpServer* server = (struct TcpServer*)arg;
	// 与客户端建立连接
	int cfd = accept(server->listener->lfd, NULL, NULL);
	// 子线程处理通信事件
	// 从线程池中取出一个子线程的反应堆实例,去处理这个cfd
	if (cfd != -1) {
		struct EventLoop* evLoop = takeWorkerEventLoop(server->threadPool);
		// 将cfd放到TcpConnection中处理
		tcpConnectionInit(cfd, evLoop);
		return 0;
	}
	return -1;
}

// 启动服务器
void tcpServerRun(struct TcpServer* server) {
	Debug("服务器程序已启动...");
	// 启动线程池
	threadPoolRun(server->threadPool);
	// 初始化一个channel实例
	struct Channel* channel = channelInit(server->listener->lfd,ReadEvent, 
	acceptConnection, NULL, NULL, server);
	// 添加检测的任务
	eventLoopAddTask(server->mainLoop, channel, ADD);
	// 启动反应堆模型
	eventLoopRun(server->mainLoop);
}