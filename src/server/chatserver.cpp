#include "chatserver.hpp"//Cmake文件构成之后，就会自动寻找了，不用加..
#include "json.hpp"
#include <functional>
#include <chatservice.hpp>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

chatserver::chatserver(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg)//传入这些参数是为了初始化网络模块
    :_server(loop,listenAddr,nameArg),
    _loop(loop)
    {
        //注册连接回调函数
        _server.setConnectionCallback(
            bind(&chatserver::onConnection, this, _1));
        //注册消息回调
        _server.setMessageCallback(bind(&chatserver::onMessage,this,_1,_2,_3));
        //设置线程数量
        _server.setThreadNum(4);
}

void chatserver::start() { _server.start(); }

void chatserver::onConnection(const TcpConnectionPtr& conn){
    //客户端断开连接
    if(!conn->connected())
    { 
        //客户端异常关闭（因为是单例，因此用作域）
        chatservice::getChatInstance()->clientCloseException(conn);
        conn->shutdown();
        }
       
}
    


void chatserver::onMessage(const TcpConnectionPtr& conn,
                Buffer* buffer,
                Timestamp time){
  string buf = buffer->retrieveAllAsString();
  //数据反序列化
  json js = json::parse(buf);
  //完全解耦网络模块和业务模块的代码
  //这里如果读取js，if判断，并实现相应操作，那么这就与网络模块强耦合了（都在网络模块中）
  //因此，这里可以根据js的key不同，调用业务模块的方法
  //因为是单例模式索引就一个对象
  //这样网络层只有两行代码，通过msgid获取业务handler =》conn，js，time
  auto msgHandler=chatservice::getChatInstance()->getHandler(js["msgid"].get<int>());
  //回调消息绑定事件处理器。来执行相应的业务处理
  msgHandler(conn, js, time);
}