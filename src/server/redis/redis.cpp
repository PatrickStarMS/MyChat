#include "redis.hpp"
#include <iostream>

using namespace std;

Redis::Redis():_publish_context(nullptr),_subcribe_context(nullptr)
{

}
Redis::~Redis()
{
    if(_publish_context !=nullptr)
    {
      redisFree(_publish_context);
    }
    if(_subcribe_context!=nullptr)
    {
      redisFree(_subcribe_context);
    }
}

bool Redis::connect()
{
    //负责publish发布消息的上下文链接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if(nullptr==_publish_context)
    {
      cerr << "connect redis failed" << endl;
      return false;
    }

    //负责subcscribe订阅消息的上下文连接
    _subcribe_context = redisConnect("127.0.0.1", 6379);
    if(nullptr==_subcribe_context)
    {
      cerr << "connect redis failed" << endl;
      return false;
    }
    //在单独的线程中监听通道上的事件,有消息给业务层上报（阻塞方式）
    thread t([&]() {observer_channel_message();});
    t.detach();
    cout << "redis connect true" << endl;
    return true;
}
 //向redis指定的通道订阅消息
 bool Redis::subscribe(int channel)
 {
    //subscribe命令本身会造成线程阻塞等待通道里面发消息，这里只订阅通道，不接受消息
    //通道消息的接受是在专门的observer_channel_message函数中的独立线程中进行
    //只负责发送命令，不阻塞接受redis server响应消息，否则notifyMsg线程会抢占应用资源
    if(REDIS_ERR==redisAppendCommand(this->_subcribe_context,"SUBSCRIBE %d",channel));
    { cerr << "subscribe command failed" << endl;
      return false;
    }
    //redisBufferWrite可以循环发送缓冲区，知道缓冲区数据发送完毕（done被设置为1）
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR==redisBufferWrite(this->_subcribe_context,&done));
        return false;
    }
    return true;
 }
 //向redis指定的通道发布消息
 bool Redis::publish(int channel, string message)
 {
    //找到通道，然后发消息（相当于在终端发送命令）
    redisReply *reply = (redisReply *)redisCommand(
        _publish_context, "PUBLISH %D %S", channel, message.c_str());
    if(reply==nullptr)
    {
      cerr << "publish message failes" << endl;
      return false;
    }
    freeReplyObject(reply);
    return true;
 }
 //向redis指定的通道取消订阅消息
 bool Redis::unsubscribe(int channel)
 {
    if(REDIS_ERR==redisAppendCommand(this->_subcribe_context,"UNSUBSCRIBE %d",channel));
    { cerr << "unsubscribe command failed" << endl;
      return false;
    }
    //redisBufferWrite可以循环发送缓冲区，知道缓冲区数据发送完毕（done被设置为1）
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR==redisBufferWrite(this->_subcribe_context,&done));
        return false;
    }
    return true;
 }
 
 //在独立线程中接受订阅消息（就是响应消息）循环阻塞的方式等待信息
 void Redis::observer_channel_message()
 {
    redisReply *reply =nullptr;
    while(REDIS_OK==redisGetReply(this->_subcribe_context,(void**)&reply))
    {
        //订阅收到的消息是一个带三个元素的数组(message,通道编号（字符串），消息内容（字符串）)
        if(reply!=nullptr&& reply->element[2]!=nullptr&&reply->element[2]->str !=nullptr)
        {
            //给业务层通报发送的消息
          _notify_message_handler(
              atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr << ">>>>>>>>>>>>>>>observer_channel_message quit<<<<<<<<<<<<<<<<<<<"
         << endl;
 }
 //初始化向业务层上报通道消息的回调对象
 void Redis::init_notify_handler(function<void(int, string)> fn)
 {
   _notify_message_handler = fn;
 }