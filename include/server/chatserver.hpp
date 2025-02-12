#ifndef CHATSERVER
#define CHATSERVER

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <functional>
using namespace muduo;
using namespace muduo::net;



// 聊天服务器由网络模块和业务模块组成
class chatserver
{
public:
    chatserver(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg);//传入这些参数是为了初始化网络模块
    ~chatserver()=default;
    void start();

private:
//连接回调
 void onConnection(const TcpConnectionPtr& conn);
 void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time);

 /* data */
 // server与loop
 TcpServer _server;//组合的moduo库，实现服务器功能的类对象
 EventLoop *_loop;//指向循环对象的指针
};

#endif // !chatserver
