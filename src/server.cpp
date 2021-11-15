#include "server.h"

char g_html_dir[8] = "./html";

const char *get_file_type(const char *name){
  const char* dot;

  // 自右向左查找‘.’字符, 如不存在返回NULL
  dot = strrchr(name, '.');   
  if (dot == NULL)
    return "text/plain; charset=utf-8";
  if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
    return "text/html";
  if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
    return "image/jpeg";
  if (strcmp(dot, ".gif") == 0)
    return "image/gif";
  if (strcmp(dot, ".png") == 0)
    return "image/png";
  if (strcmp(dot, ".css") == 0)
    return "text/css";
  if (strcmp(dot, ".au") == 0)
    return "audio/basic";
  if (strcmp( dot, ".wav" ) == 0)
    return "audio/wav";
  if (strcmp(dot, ".avi") == 0)
    return "video/x-msvideo";
  if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
    return "video/quicktime";
  if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
    return "video/mpeg";
  if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
    return "model/vrml";
  if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
    return "audio/midi";
  if (strcmp(dot, ".mp3") == 0)
    return "audio/mpeg";
  if (strcmp(dot, ".ogg") == 0)
    return "application/ogg";
  if (strcmp(dot, ".pac") == 0)
    return "application/x-ns-proxy-autoconfig";

  return "text/plain; charset=utf-8";
}

int get_line(string& http, char *buf, int size){
    char c = '\0';
    for(int i = 0; i < size - 1; i++) {
        c = http[i];
          if(c == '\r' && http[i + 1] == '\n') {
              break;
          }
          buf[i] = c;
    }
}

server::server(char* ip, unsigned short int port) : m_ep(new epoll()) {

    m_listfd = socket(PF_INET, SOCK_STREAM, 0);
    if (m_listfd < 1) {
        LOG_ERROR("socket error");
        return;
    }

    int opt = 1;
    setsockopt(m_listfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); //端口复用

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    int ret = bind(m_listfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret != 0) {
        LOG_ERROR("bind error");
        return;
    }

    ret = listen(m_listfd, 5);
    if (ret != 0) {
        LOG_ERROR("listen error");
        return;
    }
    m_ep->AddEpoll(m_listfd, EPOLLIN | EPOLLOUT);
    LOG_INFO("listen success");
}

server::~server() {
    if(m_ep){
        delete m_ep;
    }
    close(m_listfd);
}

void server::start(void){
    int i, cfd, len, ret;
    uint32_t events;
    LOG_INFO("开始wait...");
    while(1) {
        ret = m_ep->Wait();
        for(i = 0; i < ret; i++) {
            cfd = m_ep->GetEventFd(i);
            events = m_ep->GetEvents(i);
			      if(cfd == m_listfd) {
			          //客户接入处理。
                sockaddr_in client;
	              socklen_t len = sizeof(client);
                cfd = accept(m_listfd, (sockaddr*)&client, &len);
	              if(cfd < 0) {
		              LOG_ERROR("accept error");
                  continue;
	              }
                m_ep->AddEpoll(cfd, EPOLLIN);
    
            } else if( events & EPOLLIN ) {
                this->handle_event((void*)&cfd);
                
            } else if(events & EPOLLOUT) {
              LOG_INFO("触发了 epollout 事件");
            } else {
                //错误处理，直接关闭cfd。
                this->dis_connect(cfd);

            }
      }
    }
}

void server::handle_event(void* arg){
    int cfd = *(int*) arg;
    int content_length; //post参数长度
    char line[1024] = {0};
    char buf[1024] = {0};
    string http;
    //读取请求协议。
    while (true)
    {
        int len = recv(cfd, buf, sizeof(buf) - 1, 0);
        if (len == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            this->dis_connect(cfd);
            return;
        }
        else if (len == 0)
        {
            this->dis_connect(cfd);
            return;
        }
        buf[strlen(buf)] = '\0';
        http += buf;
    }
    //取出一行并分析。
    get_line(http, line, sizeof(line));
    char method[8], path[1024], protocol[12];
    sscanf(line, "%[^ ] %[^ ] %[^ ]", method, path, protocol);

    if(strncasecmp("GET", method, 3) == 0) {
        // 处理get请求。
        this->handle_get(cfd, path);
        
    } else if(strncasecmp("POST", method, 4) == 0){
        // 处理post请求。
        auto beg = http.find("Content-Length:");
		    if(beg > 0) {
			      content_length = atoi(&(http[beg + 15]));
            if(content_length > 0){
                //post方式有参数。
                char body [content_length + 1];
                strncpy(body, &(http[http.length() - content_length]), content_length);
                this->handle_post(cfd, path, body);
                return;
            }
		    }
        //post方式没参数。
        this->handle_post(cfd, path, nullptr);

    } else {
      //不支持的请求。
      this->send_file(cfd, (char*)"./html/501.html");
    }
}

void server::handle_get(int cfd, char* path){
    char file[256] = {0};
    if(strcmp(path, "/") == 0) {    
        strcpy(file, "./html/index.html");
    } else {
        snprintf(file, sizeof(file) - 1, "%s%s", g_html_dir, path);
    }
    this->send_file(cfd, file);
}

void server::handle_post(int cfd, char* path, char* body){
    char* file = path;
    if(body != nullptr) {
      //有参数。
    } else {
      //没有参数。
    }
    //还没有实现。
    strcpy(file, "./html/501.html");
    this->send_file(cfd, file);
}

void server::send_head(int fd, int no, const char* desp, const char* type, int len)
{
	char buf[256] = {'\0'};
	snprintf(buf, sizeof(buf) - 1, "HTTP/1.1 %d %s\r\n", no, desp);
  send(fd, buf, strlen(buf), 0);
  snprintf(buf, sizeof(buf) - 1,"Server: epoll webserver\r\n");
  send(fd, buf, strlen(buf), 0);
  snprintf(buf, sizeof(buf) - 1,"Content-Type: %s\r\n", type);
  send(fd, buf, strlen(buf), 0);
  snprintf(buf, sizeof(buf) - 1,"Connection: keep-alive\r\n"); //keep-alive
  send(fd, buf, strlen(buf), 0);
  snprintf(buf, sizeof(buf) - 1,"content-length: %d\r\n", len);
  send(fd, buf, strlen(buf), 0);
  snprintf(buf, sizeof(buf) - 1,"\r\n");
	send(fd, buf, strlen(buf), 0);
}

int server::get_file_len(const char* file) {
	struct stat st;
	stat(file, &st);
  return st.st_size;
}

void server::send_file(int cfd, char* file) {
	char buf[4096] = {'\0'};
  int fd = open(file, O_RDONLY);
  int len;
	if( fd == -1 ) {
    fd = open("./html/404.html", O_RDONLY);
    len = get_file_len("./html/404.html");
		this->send_head(cfd, 404, "error 404", get_file_type("./html/404.html"), len);
	} else {
    len = get_file_len(file);
    this->send_head(cfd, 200, "OK", get_file_type(file), len);
  }

	while((len = read(fd, buf,sizeof(buf))) > 0) send(cfd, buf, len, 0);
	if(len == -1)
	    LOG_ERROR("send_file read error");
}

void server::dis_connect(int fd){
  m_ep->DelEpoll(fd);
  close(fd);
}