#ifndef _HTTP_H_
#define _HTTP_H_

#include "epoll.h"

class http_conn {
    public:

        //enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATH};

        //成员函数
        http_conn();
        ~http_conn();
        void send_head(int no, const char* desp, const char* type);
        void init(int fd, sockaddr_in *addr);
        bool is_file(void);
        void send_file(void);
        void set_errfile(void);
        bool read_http(void);
        bool send_read(void);
        void send_write(void);
        void login(void);
        void reg(void);

        //关闭连接
        void conn_close(void);

        //static char m_dir[24];
        MYSQL *sql;

    private:
        //成员函数
        void init(void);

        //成员变量
        int m_cfd; //当前客户端的fd
        int m_filefd;
        int m_filelen; //返回数据长度
        struct sockaddr_in m_addr; //当前客户端的sockaddr_in
        char m_method[6]; //协议模式
        unsigned int m_cgi; //是否运行post
        int m_content_length; //post模式下body正文的长度
        string m_file; //要返回的文件路径
        //string m_query; //get模式的参数
        //string m_url;
        char m_uandp[255]; //post模式的body
        char m_id_pwd[2][32];; //用户账号和密码
        unsigned int m_err;
        string m_http;
        //unsigned int m_idx;
};

#endif