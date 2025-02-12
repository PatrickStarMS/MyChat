#ifndef CHATSERVIICE
#define CHATSERVIICE
#include <functional>
#include <unordered_map>
#include <muduo/net/TcpServer.h>
#include <json.hpp>
#include <usermodel.hpp>
#include <mutex>
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"
using namespace std;
using  namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;
//加不加变量名都行，只是为了增加可读性
using MsgHandler = std::function <
                   void(const TcpConnectionPtr& conn, json& js, Timestamp time)>;
// 单例，提供业务的代码，一个实例就够了
// 聊天服务器业务类
// 映射一个事件回调
class chatservice {
 public:
  static chatservice* getChatInstance();//单例模式获取实例对象
  //处理登陆业务
  void login(const TcpConnectionPtr& conn, json& js, Timestamp time);
  // 处理注册业务
  void reg(const TcpConnectionPtr& conn, json& js, Timestamp time);
  //一对一聊天业务
  void oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time); 
  //添加好友业务
  void addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time);
  //创建群组业务
  void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
  //加入群组业务
  void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
  //群聊业务
  void groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time);
  //获取redis的消息
  void handRedisSubscribeMessage(int channel, string message);
  // 用户退出
  void quit(const TcpConnectionPtr& conn, json& js, Timestamp time);

  // 获取消息对应的处理器
  MsgHandler getHandler(int msgid);
  //客户端异常关闭
  void clientCloseException(const TcpConnectionPtr& conn); 
  // 服务器异常，业务重置方法
  void reset();
 
 private:
  chatservice(/* args */);
  unordered_map<int, MsgHandler> _msgHandlerMap;

  //存储用户连接
  unordered_map<int, TcpConnectionPtr> _userConnection;

  //定义互斥锁，保证_userConnection的线程安全
  mutex _connectionMutex;

  // 数据操作类对象
  UserModel _userModel;
  OfflineMsgModel _offlineMsgModel;
  FriendModel _friendModel;
  GroupModel _groupModel;
  Redis _redis;
};

#endif // 