#include "http_conn.h"
#include <sys/wait.h>

const string html_file = "./html";

http_conn::http_conn() : m_cfd(-1), sql(nullptr), m_filefd(-1) {

}

http_conn::~http_conn() {
	if(m_filefd > 0) {
		close(m_filefd);
	}
}

void http_conn::init(int fd, sockaddr_in *addr) {

	//if(m_cfd != -1) return; //在使用
	bzero(&m_addr, sizeof(m_addr));
	m_addr = *addr;
	m_cfd = fd;
	this->init();
}

void http_conn::init(void) {
	//初始化一些内部成员变量 目的是覆盖前面的内容
	//m_method = 0;
	m_cgi = 0;
	m_content_length = 0;
	memset(m_uandp, '\0', sizeof(m_uandp));
	//m_query.clear();
	m_http.clear();
	m_err = 0;
	m_file.clear();
	if(m_filefd > 0) {
		close(m_filefd);
		m_filefd = -1;
	}
}

bool http_conn::send_read(void) {
	int len, i = 0;
	m_err = 0;
	if( !this->read_http() ) {
		cout << "read_http error" << m_file << endl;
		return false;
	}

	for(; m_http[i] != ' ' && i < 6; i++) {
		m_method[i] = m_http[i];
	}
	m_method[i] = '\0';
	i++; //前面循环跳出时m_http[i]是' '，要跳过' '。
	m_file.clear();
	for(; m_http[i] != ' '; i++) {
		m_file.append(&(m_http[i]), 1);
	}
	cout << "method: " << m_method << endl;

	m_cgi = 0; //防止前面数据污染
	if( !strncasecmp("GET", m_method, 3) ) {

		//如果路径只有/就默认请求index.html文件
		if(m_file  == "/") {
			m_file += "index.html";
		}

	} else if( !strncasecmp("POST", m_method, 4) ) {

		auto beg = m_http.find("Content-Length:");
		if(beg > 0) {
			m_content_length = atoi(&(m_http[beg + 15]));
		}
		//cout << "Content-Length: " << m_content_length << endl;
		if(m_content_length > 0) {
			m_http.copy(m_uandp, m_content_length, m_http.size() - m_content_length);
			m_uandp[m_content_length] = '\0';
			cout << "uandp: " << m_uandp << endl;
		}
		
		if(m_file == "/0") {
			//用户要跳转到注册界面 返回注册界面
			m_file = "/reg.html";

		} else if(m_file == "/1") {
			//用户要跳转到登录界面 返回登录界面
			m_file = "/login.html";

		} else {
			m_cgi = 1; //这个要运行cgi 置1
		}

	} else {
		//暂时没有写此类请求的处理方法
		m_err = 501;
	}
	return true;
}

void http_conn::send_write(void) {

	if(m_cgi) {
		bool ifid_pwd = false;
		int i = 0, j = 0;
		char *p = m_uandp;
		//解析body 得到账号密码
		while(*p != '\0') {
        	if(*p == '=') {
            	p++; //跳过等于号
            	ifid_pwd = true;
            	continue;
       		}
        	if(*p == '&') {
            	ifid_pwd = false;
				i++;
            	j = 0;
        	}
        	if(ifid_pwd == true) {
            	m_id_pwd[i][j++] = *p;
        	}
        	p++;
    	}
		cout << "file: " << m_file << endl;
		if( m_file == "/reg" ) {
			this->reg();

		} else if( m_file == "/login" ) {
			cout << "1111111" << endl;
			this->login();

		} else {
			m_err = 404;
		}
	}

	//返回客户端请求的文件
	if( this->is_file() ) {
		this->send_head(200, "OK", get_file_type(m_file.c_str()));
		this->send_file();
	} else {
		if(m_err < 1) {
			m_err = 404;
		}
	}

	if(m_err > 0) {
		this->set_errfile();
		if(m_err >= 600) {
			this->send_head(200, "OK", "text/html");
		} else {
			this->send_head(m_err, "OK", "text/html");
		}
		this->send_file();
	}
}

void http_conn::login(void) {
	MYSQL_RES* result = NULL;
	MYSQL_ROW row;
	char psql[255] = {'\0'};
	int ret;
	//执行mysql语句查找账号
    snprintf(psql, 255, "select * from wt1 where id='%s'", m_id_pwd[0]);
    ret = mysql_query(sql, psql);
	if(ret != 0) {
       m_err = 600;
        return;
	}
    result = mysql_store_result(sql);
	if(!result) {
		m_err = 600;
        return;
	}
    int count = mysql_num_rows(result);//获取行数
    if(count < 1) {
        //账号不存在
        m_err = 701;
        return;
	}
    
    if(!(row = mysql_fetch_row(result))) {
        //获取数据库结果集一行失败
        m_err = 600;
        return;
    }

	if(strcmp(m_id_pwd[1], row[1]) == 0) {
        //密码正确
		//memset(psql, 0, sizeof(psql));
		/*
		snprintf(psql, 255, "update wt2 set zx=1 where id='%s'", m_id_pwd[0]); //设置在线
		ret = mysql_query(sql, psql);
		if(ret != 0) {
       		m_err = 600;
        	return;
		}
		*/
        m_file = "/logincg.html"; //登录成功

    } else {
		//密码错误
        m_err = 701;
        return;
    }
}

void http_conn::reg(void) {
	MYSQL_RES* result = NULL;
	MYSQL_ROW row;
	char psql[255] = {'\0'};
	int ret;
	
	//执行mysql语句查找账号
    snprintf(psql, sizeof(psql) - 1, "select id from wt1 where id='%s'", m_id_pwd[0]);
    ret = mysql_query(sql, psql);
	if(ret != 0) {
        m_err = 600;
		return;
	}

    result = mysql_store_result(sql);
	if(!result) {
		m_err = 600;
		return;
	}

    int count = mysql_num_rows(result);//获取行数
    if(count != 0) {
        //账号已经被注册
        m_err = 700;
		return;

    } else {
        snprintf(psql, 255, "insert into wt1(id,pwd) VALUES('%s','%s')", m_id_pwd[0], m_id_pwd[1]);
	    if(mysql_query(sql, psql)) {
            m_err = 600;
			return;
	    }
		m_file = "/regcg.html"; //注册成功
    }
}

void http_conn::conn_close(void) {
	cout << "close" << endl;
	//sql == nullptr;
}