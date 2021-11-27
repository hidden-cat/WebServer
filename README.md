# WebServer

一个单线程的、linux系统下的web服务器

使用c++语言 + epoll

server为主要类，大部分功能在此实现

epoll类主要封装了epoll

log类为日志输出

开发环境为: debian 9 and g++ 6.3.0

编译直接执行make ser 编译后执行./ser -i0.0.0.0 -p5200. 参数i是ip地址，p是端口号

多线程版本https://github.com/hidden-cat/WebServer_Thread
