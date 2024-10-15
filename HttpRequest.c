#define _GNU_SOURCE
#include "HttpRequest.h"
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>

#define HeaderSize 12
// 初始化
struct HttpRequest* httpRequestInit() {	
	struct HttpRequest* request = (struct HttpRequest*)malloc(sizeof(struct HttpRequest));
	httpRequestReset(request);
	request->reqHeaders = (struct RequestHeader*)malloc(sizeof(struct RequestHeader) * HeaderSize);
	return request;
}

// 重置
void httpRequestReset(struct HttpRequest* req) {
	req->curState = ParseReqLine;
	req->method = NULL;
	req->url = NULL;
	req->version = NULL;
	req->reqHeadersNum = 0;
}

void httpRequestResetEx(struct HttpRequest* req) {
	free(req->url);
	free(req->method);
	free(req->version);
	if (req->reqHeaders != NULL) {
		for (int i = 0; i < req->reqHeadersNum; i++) {
			free(req->reqHeaders[i].key);
			free(req->reqHeaders[i].value);
		}
		free(req->reqHeaders);
	}
	httpRequestReset(req);
}

void httpRequestDestroy(struct HttpRequest* req) {
	if (req != NULL) {
		httpRequestResetEx(req);	
		free(req);
	}
}

// 获取处理状态
enum HttpRequestState httpRequestState(struct HttpRequest req) {
	return req.curState;
}

// 添加请求头
void httpRequestAddHeader(struct HttpRequest* req, const char* key, const char* val) {
	req->reqHeaders[req->reqHeadersNum].key = (char*)key;
	req->reqHeaders[req->reqHeadersNum].value = (char*)val;
	req->reqHeadersNum++;
}

// 根据key得到val
char* httpRequestGetHeader(struct HttpRequest* req, const char* key) {
	if (req != NULL) {
		for (int i = 0; i < req->reqHeadersNum; i++) {
			if (strncasecmp(req->reqHeaders[i].key, key, strlen(key)) == 0) {
				return req->reqHeaders[i].value;
			}
		}
	}
	return NULL;
}

// 拆分请求行
char* splitRequestLine(const char* start, const char* end, const char* sub, char** ptr) {
	char* space = (char*)end;
	if (sub != NULL) {
		space = memmem(start, end - start, sub, strlen(sub));
		assert(space != NULL);
	}
		int length = space - start;
		// printf("line85: space: %o start: %o length: %d \n", space, start, length);
		char* tmp = (char*)malloc(length + 1);
		assert(tmp != NULL);
		strncpy(tmp, start, length);
		tmp[length]= '\0';
		*ptr = tmp;
		return space + 1;
}

// 解析请求行
bool parseHttpRequestLine(struct HttpRequest* request, struct Buffer* readBuf) {
	// 读出请求行
	char* end = bufferFindCRLF(readBuf);
	// 保存字符串起始地址
	char* start = readBuf->data + readBuf->readPos;
	// 请求行总长度
	int lineSize = end - start;
	// printf("line102 lineSize: %d start: %o end: %o \n", lineSize, start, end);

	if (lineSize > 0) {
		start = splitRequestLine(start, end, " ", &request->method);
		start = splitRequestLine(start, end, " ", &request->url);
		splitRequestLine(start, end, NULL, &request->version);
#if 0
		// get /xxx/xxx.txt http/1.1
		// 请求方式
		char* space = memmem(start, lineSize, " ", 1);
		assert(space != NULL);
		int methodSize = space - start;
		request->method = (char*)malloc(methodSize + 1);
		strncpy(request->method, start, methodSize);
		request->method[methodSize] = '\0';

		// 请求大静态资源
		start = space + 1;
		char* space = memmem(start, end - start, " ", 1);
		assert(space != NULL);
		int urlSize = space - start;
		request->url = (char*)malloc(urlSize + 1);
		strncpy(request->url, start, urlSize);
		request->url[urlSize] = '\0';

		// http版本
		start = space + 1;
		// char* space = memmem(start, end - start, " ", 1);
		// assert(space != NULL);
		// int urlSize = space - start;
		request->version = (char*)malloc(end - start + 1);
		strncpy(request->url, start, end - start);
		request->version[end - start + 1] = '\0';
#endif

		// 为解析请求头做准备
		readBuf->readPos += lineSize;
		readBuf->readPos += 2;
		// 修改状态
		request->curState = ParseReqHeaders;
		return true;
	}
	return false;
}

// 解析请求头
bool parseHttpRequestHeader(struct HttpRequest* request, struct Buffer* readBuf) {
	char* end = bufferFindCRLF(readBuf);
	if (end != NULL) {
		char* start = readBuf->data + readBuf->readPos;
		int lineSize = end - start;
		// 基于:搜索字符串
		char* middle = memmem(start, lineSize, ": ", 2);
		if (middle != NULL) {
			char* key = malloc(middle - start + 1);
			strncpy(key, start, middle - start);
			key[middle - start] = '\0';

			char* value = malloc(end - middle - 2 + 1);
			strncpy(value, middle + 2, end - middle - 2);
			value[end - middle - 2] = '\0';

			httpRequestAddHeader(request, key, value);
			// 移动读数据的位置		
			readBuf->readPos += lineSize;
			readBuf->readPos += 2;
		}
		else {
			// 请求头被解析完了,跳过空行
			readBuf->readPos += 2;
			// 修改解析状态
			// 忽略post请求,按照get请求处理
			request->curState = ParseReqDone;
		}
		return true;
	}
	return false;
}

// 解析http请求协议
bool parseHttpRequest(struct HttpRequest* request, struct Buffer* readBuf, 
struct HttpResponse* response, struct Buffer* sendBuf, int socket) {
	bool flag = true;
	while (request->curState != ParseReqDone) {
		switch (request->curState) {
		case ParseReqLine:
			flag = parseHttpRequestLine(request, readBuf);
		break;
		case ParseReqHeaders:
			flag = parseHttpRequestHeader(request, readBuf);
		break;
		case ParseReqBody:
			// 默认均为get 不处理
		break;
		default:
		break;
		}
		if (!flag) {
			return flag;
		}
		// 判断是否解析完毕,若完毕,需要准备回复的数据
		if (request->curState == ParseReqDone) {
			// 1. 根据解析出的原始数据, 对客户端的请求作出处理
			processHttpRequest(request, response);
			// 2. 响应数据并发送给客户端
			httpResponsePrepareMsg(response, sendBuf, socket);
		}
	}	
	// 还原状态, 保证还能处理后续请求
	request->curState = ParseReqLine;
	return flag;
}

// 将字符转换为整形数
int hexToDec(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
}

// 解码  to 用于存储解码后的数据,传出参数 from要被解码的数据,传入参数
void decodeMsg(char* to, char* from) {
	for (; *from != '\0'; to++, from++) {
		// 	判断字符是否为%+十六进制数
		// ps: "Linux%E5%86%85%E6%A0%B8.jpg"
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
			// 将16进制转换为10进制 在将类型进行转换 int -> char
			// ps: %B2 = 178
			// 将三个字符，变成一个字符，这个字符就是原始数据
			*to = hexToDec(from[1]) * 16 + hexToDec(from[2]);
			from += 2;
		}
		else {
			// 字符拷贝
			*to = *from;
		}
	}
	*to = '\0';
}

// 处理http请求协议
bool processHttpRequest(struct HttpRequest* request, struct HttpResponse* response) {
	if (strcasecmp(request->method, "get") != 0) {
		return -1;
	}
	// 根据uef-8 解码
	decodeMsg(request->url, request->url);
	// 处理客户端请求的资源(目录/文件)
	char* file = NULL;
	if (strcmp(request->url, "/") == 0) {
		file = "./";
	}
	else {
		file = request->url + 1;
	}
	// 获取文件属性
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1) {
		// 文件不存在 -- 回复404
		// sendHeadMsg(cfd, 404, "Not Foutd", getFileType(".html"), -1);
		// sendFile("404.html", cfd);
		strcpy(response->fileName, "404.html");
		response->statuseCode = NotFound;
		strcpy(response->statusMsg, "Not Fount");
		// 响应头
		httpResponseAddHeader(response, "Content-type", getFileType(".html"));
		response->sendDataFunc = sendFile;
		return 0;
	}
	strcpy(response->fileName, file);
	response->statuseCode = OK;
	strcpy(response->statusMsg, "OK");
	// 判断文件类型
	if(S_ISDIR(st.st_mode)) {
		// 发送目录给客户端
		// 响应头
		httpResponseAddHeader(response, "Content-type", getFileType(".html"));
		response->sendDataFunc = sendDir;
	}
	else {
		// 发送文件内容给客户端
		// 响应头
		char tmp[12] = {0};
		sprintf(tmp, "%ld", st.st_size);
		httpResponseAddHeader(response, "Content-type", getFileType(file));
		httpResponseAddHeader(response, "Content-length", tmp); 
		response->sendDataFunc = sendFile;
	}
	return false;
}

// 获取文件类型对应的type
const char* getFileType(const char* name) {
	// a.jpg a.mp4 a.html
	// 自右向左查找'.'字符，若不存在返回NULL
	const char* dot = strrchr(name, '.');
	// 纯文本
	if (dot == NULL) 
		return "text/plain; charset=utf-8";
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "image/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "medel/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/mmidi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".mp4") == 0)
		return "video/mpeg4";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

		return "text/plain; charset=utf-8";
}

// 发送目录
void sendDir(const char* dirName, struct Buffer* sendBuf, int cfd) {
	// 要发送的内容
	char buf[4096] = { 0 };
	// 拼接起始标签
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
	// 使用scandir获取目录中文件的名字
	struct dirent** nameList;
	// 内部会出现内存创建 后续需要释放
	int dirNum = scandir(dirName, &nameList, NULL, alphasort);
	for (int i = 0; i < dirNum; i++) {
		// 取出文件名 nameList是一个指针数组 struct dirnet* tmp[]
		char* name = nameList[i]->d_name;
		struct stat st;
		// 获取文件名
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName, name);
		stat(subPath, &st);
		// 拼接后续与要点击文件相关内容
		if (S_ISDIR(st.st_mode)) {
			// a标签 <a href="url">name</a>
			sprintf(buf + strlen(buf),
			 "<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>", 
			 name, name, st.st_size);
		}
		else {
			sprintf(buf + strlen(buf),
			 "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>", 
			 name, name, st.st_size);
		}
		// send(cfd, buf, strlen(buf), 0);
		bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
		bufferSendData(sendBuf, cfd);
#endif
		memset(buf, 0, sizeof(buf));
		free(nameList[i]);
	}
	// 拼接发送结束标签
	sprintf(buf, "</table></body><html>");
	// send(cfd, buf, strlen(buf), 0);
	bufferAppendString(sendBuf, buf);
#ifndef MSG_SEND_AUTO
	bufferSendData(sendBuf, cfd);
#endif
	free(nameList);
}

// 发送文件
void sendFile(const char* fileName, struct Buffer* sendBuf, int cfd) {
	// 打开文件
	int fd = open(fileName, O_RDONLY);
	assert(fd > 0);

#if 2
	while (1) {
		char buf[1024];
		int len = read(fd, buf, sizeof(buf));
		if (len > 0) {
			// send(cfd, buf, len, 0);
#ifndef MSG_SEND_AUTO
			bufferAppendData(sendBuf, buf, len);
#endif
			bufferSendData(sendBuf, cfd);
			// 这非常重要: 留给客户端10微秒用于处理数据
			// usleep(10);
		}
		else if (len == 0) {
			break;
		}
		else {
			close(fd);
			perror("read");
		}
	}
#else
	off_t offset = 0;
	int size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	while (offset < size) {
		int ret = sendfile(cfd, fd, &offset, size - offset);
		// perror("sendfile:");
		usleep(1000);
		// printf("ret value: %d\n", ret);
		if (ret == -1 && errno == EAGAIN) {
			// printf("没有数据...\n");
		}
	}
#endif
	close(fd);
}