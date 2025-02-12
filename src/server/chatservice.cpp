#include "chatservice.hpp"
#include <public.hpp>
#include <muduo/base/Logging.h>
#include <string>
#include <vector>

using namespace muduo;
using namespace std;
using namespace placeholders;

chatservice* chatservice::getChatInstance()
{
    static chatservice service;
    return &service;
}
//方法
void chatservice::reset()
{
    // 把所有online用户的状态置为offline
    _userModel.resetState();
}

//客户端正常退出
void chatservice::quit(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
  
  int userid = js["id"].get<int>();
  {
  lock_guard<mutex> lock(_connectionMutex);
  auto it = _userConnection.find(userid);
  
  if(it!=_userConnection.end())
    {
      
      _userConnection.erase(it);
    }
  }
  //2.更新用户的状态信息
  User user(userid, "", "", "offline");
  user.setState("offline");
  _userModel.updateState(user);

  //下线要取消订阅
  _redis.unsubscribe(userid);
}
//客户端异常退出
void chatservice::clientCloseException(const TcpConnectionPtr& conn)
{
  User user;
  {
    lock_guard<mutex> lock(_connectionMutex);
    //1.
    for (auto it = _userConnection.begin(); it != _userConnection.end(); ++it) 
    {
      if(it->second==conn)
      {
        user.setId(it->first);
        _userConnection.erase(it);
        break;
      }
    }
  }
  //2.更新用户的状态信息
  if(user.getId()!=-1)
  {
    user.setState("offline");
    _userModel.updateState(user);
  }
  //下线要取消redis中的订阅
  _redis.unsubscribe(user.getId());
}

//业务

//注册消息以及对应的Handler回调操作
chatservice::chatservice()
{
    //成员变量初始化
    _msgHandlerMap.insert({LOGIN_MSG,bind(&chatservice::login,this,_1,_2,_3)});
    _msgHandlerMap.insert({REG_MSG,bind(&chatservice::reg,this,_1,_2,_3)});
    _msgHandlerMap.insert({ ONE_CAHT_MSG,bind(&chatservice::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert(
        {ADD_FRIEND_MSG, bind(&chatservice::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert(
        {LOGINOUT_MSG, bind(&chatservice::quit, this, _1, _2, _3)});
  if(_redis.connect())
  {
    //设置上报消息的回调
    _redis.init_notify_handler(
        bind(&chatservice::handRedisSubscribeMessage, this, _1, _2));
  }
}

void chatservice::handRedisSubscribeMessage(int channel,string message)
{
  

  lock_guard<mutex> lock(_connectionMutex);
  //cahnnel就是userid
  auto it = _userConnection.find(channel);
  if(it!=_userConnection.end())
  {
    it->second->send(message.c_str());
    return;
  }
  //可能在推送消息的时候还是在线的，当把消息推过来的时候，下线了
  _offlineMsgModel.insert(channel, message);
}

MsgHandler chatservice:: getHandler(int msgid)
{
  auto it =_msgHandlerMap.find(msgid);
  if(it==_msgHandlerMap.end())
  {
    //使用的是muduo库的log打印
    // LOG_ERROR << "msgid" << msgid << "can not find handler";
    //这里如果异常，返回空操作
    return [=](const TcpConnectionPtr& conn, json& js, Timestamp time){
        LOG_ERROR << "msgid" << msgid << "can not find handler";
    };
  } else
    return _msgHandlerMap[msgid];
}

//处理登陆业务 ip pwd 对比和数据库中的ip和pwd
void chatservice::login(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
  LOG_INFO << "do login service";
  int id = js["id"].get<int>();
  string password = js["password"];
  User user = _userModel.query(id);
  
  if (user.getId() == id && user.getPassword() == password) {
    //用户存在，但是已经登录，不允许重复登陆,那这样登陆成功的时候，再记录数据库数据之后，要改变状态
    if(user.getState()=="online")
    {
      
      json response;
      response["mesgid"] = LOGIN_MSG_ACK;
      response["errno"] = 2;//已经登陆
      response["errmsg"] = "用户已经登陆";
      conn->send(response.dump());
    }
  else{
      //登陆成功，记录用户连接信息
      {
         lock_guard<mutex> lock(_connectionMutex);
        _userConnection.insert({id, conn});
      }

      //id用户登陆成功后，向redis订阅channel
      _redis.subscribe(id);

      // 登陆成功
      user.setState("online");
      _userModel.updateState(user);
      json response;

      response["mesgid"] = LOGIN_MSG_ACK;
      response["errno"] = 0;//已经登陆
      response["id"] = user.getId();
      response["name"] = user.getName();//一般来说，昵称啥的都记录在本地，这里先这样写
      //查询该用户是否有离线消息
      vector<string> vec = _offlineMsgModel.query(id);
      if(!vec.empty())
      {
        response["offlinemsg"] = vec;
        //从表中删除离线消息
        _offlineMsgModel.remove(id);
      }


      //查询该用户的好友信息并返回
      vector<User> userVec = _friendModel.query(id);
      if(!userVec.empty())
      {
        //这里是要考虑是否使用同一个js，如果使用同一个的话，send之后就所有的消息乱了
        //这里选择先建一个js，然后作为另一个js的js
        vector<string> vec2;
        for(User &user:userVec)
        {
          json js;
          js["id"]=user.getId();
          js["name"]=user.getName();
          js["state"] = user.getState();
          vec2.push_back(js.dump());
        }
        response["friends"] = vec2;
      }

      //查询用户的群组消息
       vector<Group> groupuserVec = _groupModel.queryGroup(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getGroupUser())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }
      conn->send(response.dump());
  }

  } 
  else {
    //登陆失败（其实还可以细分成，用户存在，密码错误；与用户不错在
    json response;
    response["mesgid"] = LOGIN_MSG_ACK;
    response["errno"] = 1;
    response["errmsg"] = "用户名密码错误";
    conn->send(response.dump());
  }
}
// 处理注册业务
void chatservice::reg(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    LOG_INFO << "do reg service";
    string name = js["name"];
    string passward = js["password"];
  //将客户端传过来的注册信息保存到user对象中，然后通过user的操作类，将user存的信息发送到数据库
  //并使用json回应客户端 
    User user;
    user.setName(name);
    user.setPassword(passward);
    bool state = _userModel.insert(user);
    //根据是否将注册消息成功写入数据库，分情况，回复客户端
    if(state)
    {
      //注册成功,给客户端回消息
      json response;
      response["mesgid"] = REG_MSG_ACK;
      response["errno"] = 0;
      response["id"] = user.getId();
      conn->send(response.dump());
    } 
    else 
    {
      //注册失败，给客户端回消息
      json response;
      response["mesgid"] = REG_MSG_ACK;
      response["errno"] = 1;
      conn->send(response.dump());
    }
}



void chatservice::oneChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
  int toid = js["to"].get<int>();

 
  // 找id所对应的连接
  { 
    lock_guard<mutex> lock( _connectionMutex);
    auto it = _userConnection.find(toid);
    if(it != _userConnection.end() )
      {
        //在线，转发消息,服务器做了消息中转,主动推送消息给toid用户
        it->second->send(js.dump());
        return;
    }
  }
  //从数据库查询toid,在线的话就向redis发消息
  User user=_userModel.query(toid);
  if(user.getState()=="online")
  {
    _redis.publish(toid, js.dump());
    return;
  }
  // 不在线，存储消息
  _offlineMsgModel.insert(toid, js.dump());
}

//添加好友业务,msgid(哪个业务) id friendid
void chatservice::addFriend(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
  int userid = js["id"].get<int>();
  int friendid = js["friendid"].get<int>();
  //添加好友
  _friendModel.insert(userid, friendid);

}



//创建群组业务
void chatservice::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
  //解析传过来的js
  int userid = js["id"].get<int>();
  string name = js["groupname"];
  string desc = js["groupdesc"];

  //存储新创建的群组消息
  Group group(-1, name, desc);
  if(_groupModel.createGroup(group))
  {
    //存储群组创建人信息
    _groupModel.addGroup(userid, group.getId(), "creator");
  }
}


//加入群组业务
void chatservice::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
   //解析传过来的js
  int userid = js["id"].get<int>();
  int groupid = js["groupid"].get<int>();


    //存储群组创建人信息
    _groupModel.addGroup(userid, groupid, "normal");
  
}

//群组聊天
void chatservice::groupChat(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
  //查询群组人员，然后转发就是了
  //转发获取连接，访问队列要加锁
  int userid =js["id"].get<int>();
  int groupid = js["groupid"].get<int>();
  //存放查到的id
  vector<int> useridVec = _groupModel.queryGroupUser(userid, groupid);
  for(int id:useridVec)
  {
    lock_guard<mutex> lock(_connectionMutex);
    auto it = _userConnection.find(id);
    if(it!=_userConnection.end())
    {
      //转发消息
      it->second->send(js.dump());
    }
    else
    {
      //从数据库查询toid,在线的话就向redis发消息
    User user=_userModel.query(id);
    if(user.getState()=="online")
    {
      _redis.publish(id, js.dump());
      
    }
    else
     { //存储离线消息
      _offlineMsgModel.insert(id, js.dump());
     }
    }
  }
}


