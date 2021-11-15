#include "log.h"
#include "server.h"

static server* g_ser;

void close_process(int msg) {
    if(g_ser){
        delete g_ser;
    }
    g_ser = NULL;
    exit(0);
}

int main(int argc, char *argv[]) {

    //屏蔽信号。
    for(int i = 0; i < 50; i++) {
        signal(i, SIG_IGN);
    }
    //退出信号不能屏蔽 重新监视起来。
    signal(SIGTERM, close_process);
    signal(SIGINT,close_process);

    char ip[16] = {'\0'};
    unsigned short int port = 0;
    int ret;
    log* l = log::get_instance();
    if( !l->init("log", 200, 500) ) {
        cout << "log create error" << endl;
    }
    //解析参数。
    while ((ret = getopt(argc, argv, "i:p:")) != -1) {
        
        switch(ret) {

            case 'i' :
                strcpy(ip, optarg);
                break;
            case 'p' :
                port = (unsigned short int)atoi(optarg);
                break;
        }
    }
    cout << ip << "   " << port << endl;
    chdir("/home/w123/Desktop/c_and_c++_code_new/c++/lx/c++httptest/");
    g_ser = new server(ip, port);
    g_ser->start();
    return 0;
}