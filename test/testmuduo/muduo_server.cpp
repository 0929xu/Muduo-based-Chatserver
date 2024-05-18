/*
muduo网络库给用户提供了两个主要的类
TcpServer：用于编写服务器程序
TcpClient：用于编写客户端程序

epoll + 线程池
好处：能够把网络I/O代码和业务代码分开
我们只需要关注两件事：1. 用户的连接和断开 2. 用户的可读写事件
网络库帮我们处理好的：1. 什么时候发生这个事件 2. 如何监听事件的发生
*/

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>  // 绑定器bind在<functional>里
#include <string>
using namespace std;
using namespace muduo::net;
using namespace muduo;
using namespace placeholders;

/* 基于muduo网络库开发服务器程序
1. 组合TcpServer对象
2. 创建EventLoop事件循环对象的指针
3. 明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
4. 在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5. 
*/
class ChatServer 
{
public:
    ChatServer(EventLoop* loop,  // 事件循环
                const InetAddress& listenAddr,   // IP+Port
                const string& nameArg)  // 服务器名字
        : _server(loop, listenAddr, nameArg), _loop(loop) 
    {
        /*
        普通函数回调，比如说func()，这是我知道事件什么时候发生，发生了该怎么做
        但是很多时候事件发生和发生后该怎么做不在同一时刻，就比如用户的创建和已创建的用户断开，还有用户读写事件
        我们不知道什么时候有用户创建，什么时候用户会断开连接，得通过网络对端发送相应的数据，上报上来才知道
        网络库帮我监听这些事件，我现在只知道发生这些事件我该做什么
        所以我需要去注册事件回调，当相应的事件发生后，网络库会帮我调用回调函数，我只关注在回调函数里面开发我的业务就好了
        */

        // 给服务器注册用户连接的创建和断开回调，当有连接的创建和断开的时候，会帮我们调用onConnection()方法
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

        // 给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        // 设置服务器端的线程数  1个I/O线程  3个worker线程
        _server.setThreadNum(4);
        
    }

    // 开启事件循环
    void start()
    {
        _server.start();
    }

private:
    // 我们主要关注在 onConnection 和 onMessage 的内容怎么写
    // 专门处理用户的连接创建和断开 epoll listenfd accept
    void onConnection(const TcpConnectionPtr& conn) 
    {
        if (conn->connected())  // 返回一个bool值，true就是连接成功了
        {
            // peer 对端  toIpPort 返回Ip地址和端口号
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state: online " << endl;
        }
        else {
            cout << conn->peerAddress().toIpPort() << " -> " << conn->localAddress().toIpPort() << " state: offline " << endl;
            conn->shutdown();  // close(fd)
            // _loop->quit();
        }
    }

    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr& conn,   // 连接
                    Buffer* buffer,                // 缓冲区，提高数据收发的性能
                    Timestamp time)                // 接收到数据的时间信息
    {
        string buf = buffer->retrieveAllAsString();   // 接收信息
        cout<<"recv data:" << buf << " time:" << time.toString() <<endl;
        conn->send(buf);  // 回声echo，收到什么回什么
    }

    TcpServer _server;  // #1
    EventLoop *_loop;   // #2 epoll
};

int main() 
{
    EventLoop loop;  // epoll
    InetAddress addr("192.168.179.128", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();  // 启动服务 listenfd epoll_ctl=>epoll
    loop.loop();    // epoll_wait 以阻塞的方式等待新用户连接，已连接用户的读写事件等

    return 0;
}