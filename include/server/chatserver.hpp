#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

// 聊天服务器的主类
class ChatServer
{
public:
    // 构造函数 准备构造函数是为了它的成员变量_server ，因为TcpServer没有默认构造函数
    // 初始化聊天服务器对象
    ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg);
    
    // 启动服务
    void start();


private:
    // 上报链接相关信息的回调函数
    void onConnection(const TcpConnectionPtr&);

    // 上报读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr&,
                            Buffer*,
                            Timestamp);

    TcpServer _server; // 组合的muduo库，实现服务器功能的类对象
    EventLoop *_loop;  // 指向事件循环对象的的指针


};


#endif