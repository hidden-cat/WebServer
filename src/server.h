#ifndef _SERVER_H_
#define _SERVER_H_
#include <iostream>
#include <sys/stat.h>
#include <set>
#include "epoll.h"

class server {

public:
	server(char* ip, unsigned short int port);
	~server();
	//服务器运行。
	void start(void);
	//客户端发来信息由此事件处理。
	void handle_event(void* arg);
	//处理get方法。
	void handle_get(int cfd, char* path);
	//处理post方法(未实现)。
	void handle_post(int cfd, char* path, char* body);
	//发送协议头。
	void send_head(int fd, int no, const char* desp, const char* type, int len);
	//返回文件长度。
	int get_file_len(const char* file);
	//发送文件给客户端。
	void send_file(int cfd, char* file);
	//关闭连接。
	void dis_connect(int fd);

private:
	int m_listfd;
	epoll* m_ep;
};

#endif

