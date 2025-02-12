#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include<vector>
#include<chrono>
#include <ctime>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

//记录当前系统登陆的用户信息
User g_currentUser;
//记录当前登录用户的好友列表
vector<User> g_currentUserFriendList;
//记录当前登录用户的群组列表,群组group里面有vector<GroupUser> user;，每个GroupUser都是继承的User，并附加角色
vector<Group> g_currentUserGroupList;

//用于线程间通信
sem_t rwsem;
//记录登陆状态
atomic_bool g_isLoginSuccess(false);

void help(int fd=0, string str="");
void chat(int clientfd, string str);
void addfriend(int clientfd, string str);
void creategroup(int clientfd, string str);
void addgroup(int clientfd, string str);
void groupchat(int clientfd, string str);
void quit(int clientfd, string str);
// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
  {"help","显示所有支持的命令,格式help"},
  {"chat","一对一聊天,格式chat:friendid:message"},
  {"addfriend","添加好友,格式addfriend:friendid"},
  {"creategroup","创建群组,格式creategroup:groupname:groupdesc"},
  {"addgroup","加入群组,格式addgroup:groupid"},
  {"groupchat","群聊,格式groupchat:groupid:message"},
  {"quit","注销,格式quit"}

};
// 输入一个命令（字符串），然后和存在map中的string（key）比较，然后调用对应的函数--字符串到函数的映射
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"quit", quit}

};
bool isMainMenuRunning = false;

//显示当前登录成功用户的基本信息
void showCurrentUserData();
//接收线程(接受用户的手动输入)
void readTaskHandler(int clientfd);
//获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
//聊天页面程序
void mainMenu(int clientfd);

//聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc,char **argv)
{
    if(argc<3)
    {
      cerr << "commond invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
      exit(-1);
    }
    //解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    //创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1==clientfd)
    {
      cerr << "socket create error" << endl;
      exit(-1);
    }

    //填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    //client和server连接
    if(-1==connect(clientfd,(sockaddr*)&server,sizeof(sockaddr_in)))
    {
      cerr << "connect server error" << endl;
      close(clientfd);
      exit(-1);
    }

  //初始化读写线程通信用的信号量
    sem_init(&rwsem, 0, 0);

    //连接服务器成功，启动链接子线程
    thread readTask(readTaskHandler, clientfd);
    readTask.detach();//线程执行完，内核自己回收

    //main线程用于接受用户的输入，负责发送数据
    //死循环，用户没有登录或者退出就一直在这里
    for (;;) 
    {
           //显示首页菜单 登录 注册 退出
           cout << "===================" << endl;
           cout << "1. login" << endl;
           cout << "2. register" << endl;
           cout << "3. quit" << endl;
           cout << "choice:";
           int choice = 0;
           cin >> choice;
           cin.get();//读掉缓冲区残留的回车
           switch(choice)
           {
            case 1://login业务
            {
              int id = 0;
              char pwd[50] = {0};
              cout << "userid:";
              cin >> id;
              cin.get();//读掉缓冲区的残留回车
              cout << "password:";
              cin.getline(pwd, 50);

              json js;
              js["msgid"] = LOGIN_MSG;
              js["id"] = id;
              js["password"] = pwd;
              string request = js.dump();

              g_isLoginSuccess = false;

              int len = send(clientfd, request.c_str(),
                             strlen(request.c_str()) + 1, 0);
             
              if (len == -1) {
                cerr << "fial to send" << endl;
                exit(-1);
              }

              sem_wait(&rwsem);//等待子线程的信号量
              if(g_isLoginSuccess==true)
              {
                isMainMenuRunning = true;
                // 进入聊天主菜单页面
                mainMenu(clientfd);
              }

              
            }   
            break;
            case 2://注册业务简单
            {
                //输入用户名与密码即可
                char name[50] = {0};
                char pwd[50] = {0};
                cout << "username:";
                cin.getline(name,50);
                cin.get();
                cout << "password:";
                cin.getline(pwd, 50);
                cin.get();
                
                //将消息转成js，并序列化
                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();
                // 将序列化的js从客户端发给服务器，之前直接输入，现在要通过socket
                // 往哪里发，发什么，发多长，标志位没有特殊状态
                int len =
                    send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if(len==-1)
                {
                  cerr << "send reg msg error:" << request << endl;
                }
                sem_wait(&rwsem);

            } break;
            case 3:  // quit
            {
              close(clientfd);
              sem_destroy(&rwsem);
              exit(0);
            } 
             
            default:
              cerr << "invalid input!" << endl;
              break;
           }

    }
    return 0;
}


//处理登陆响应，引用传递
void doLoginResponse(json &responsejs)
{
  
      if(0!=responsejs["errno"].get<int>())//登陆失败
      {
        cerr << responsejs["errmsg"] << endl;
        cerr << responsejs["errno"] << endl;
        g_isLoginSuccess = false;
      } else {
        // 一登陆上，服务端自定义好的给返回
        //  记录当前用户的id和name
        g_isLoginSuccess = true;
        
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

      //记录当前用户的好友和群组信息
      if(responsejs.contains("friends"))
      {
        
        //初始化，清空，防止重复添加
        g_currentUserFriendList.clear();

        vector<string> vec = responsejs["friends"];

        for(string &str :vec)
        {
          //转成js好操作
          json js = json::parse(str);
          User user;
          user.setId(js["id"].get<int>());
          user.setName(js["name"]);
          user.setState(js["state"]);
          g_currentUserFriendList.push_back(user);
        }
      }

      //记录当前用户的群组信息
      if(responsejs.contains("groups"))
      {
        g_currentUserGroupList.clear();
        vector<string> vec1 = responsejs["groups"];
        for(string &groupstr :vec1)
        {
          json groupjs = json::parse(groupstr);
          Group group;
          group.setId(groupjs["id"].get<int>());
          group.setName(groupjs["name"]);
          group.setDesc(groupjs["groupdesc"]);

          //群组信息中还有群成员
          //上面之反序列化一层
          vector<string> vec2= groupjs["users"];
          for(string &userstr:vec2)
          {
            json userjs = json::parse(userstr);
            //群成员还有别的身份，因此不能用User
            GroupUser user;
            user.setId(userjs["id"].get<int>());
            user.setName(userjs["name"]);
            user.setState(userjs["state"]);
            user.setRole(userjs["role"]);
            //放一个群组中的成员
            group.getGroupUser().push_back(user);
          }
          //放群组的
          g_currentUserGroupList.push_back(group);
        }
      }

      //展示好友和群组信息
      showCurrentUserData();

      //离线消息要返回
      if(responsejs.contains("offlinemsg"))
      {
        vector<string> vec = responsejs["offlinemsg"];
        for(string &offlinemsgstr : vec)
        {
          json js = json::parse(offlinemsgstr);

            if(ONE_CAHT_MSG==js["msgid"].get<int>())//所有的聊天消息都是基于一对一的
            {
              //打印聊天消息
              cout << js["time"].get<int>() << " [" << js["id"].get<int>() << "] "
                  << js["name"].get<string>() << "said: " << js["msg"].get<string>()
                  << endl;
              
            }
            else 
            {
              cout << "群消息"<<js["groupid"]<<":"<<js["time"].get<int>() << " [" << js["id"].get<int>() << "] "
                  << js["name"].get<string>() << "said: " << js["msg"].get<string>()
                  << endl;
              
            }
          
        }
      }
      }
}

//处理注册逻辑
void doRegResponse(json &responsejs)
{
  
  
  if(responsejs["errno"].get<int>()!=0)//自己在服务端这值得errno为0，代表成功
  {
    //注册失败
    cerr <<  "is fail to regist,due already exist" << endl;
  }
  else
  {
    cerr << " success to registe,id is: "
          << responsejs["id"] << ",do not forget it" << endl;

  }
}

//接收线程
void readTaskHandler(int clientfd) 
{ 
  //线程一般都是死循环，一直执行某个任务
  for (;;) 
  {
    char buff[1024] = {0};
    int len = recv(clientfd, buff, 1024, 0);
    if(-1==len||len==0)
    {
      close(clientfd);
      exit(-1);
    }

    json js = json::parse(buff);
    //判断消息类型是什么消息
    if(ONE_CAHT_MSG==js["msgid"].get<int>())//所有的聊天消息都是基于一对一的
    {
      //打印聊天消息
      cout << js["time"].get<int>() << " [" << js["id"].get<int>() << "] "
           << js["name"].get<string>() << "said: " << js["msg"].get<string>()
           << endl;
      continue;
    }
    if(GROUP_CHAT_MSG==js["msgid"].get<int>())
    {
      cout << "群消息"<<js["groupid"]<<":"<<js["time"].get<int>() << " [" << js["id"].get<int>() << "] "
           << js["name"].get<string>() << "said: " << js["msg"].get<string>()
           << endl;
      continue;
    }
    //处理登陆的消息
    if(LOGIN_MSG_ACK==js["msgid"].get<int>())
    {
      //将主线程的搬过来，由于太长了，先封装一下
      //无非就是读出数据，然后判断处理
      doLoginResponse(js);
      //处理完通知主线程
      sem_post(&rwsem);
      continue;
    }
    if(REG_MSG_ACK==js["msgid"].get<int>())
    {
      doRegResponse(js);
      //处理完通知主线程
      sem_post(&rwsem);
      continue;
    }
  }
}
void showCurrentUserData()
{
  cout << "==================login in ======================" << endl;
  cout << "current login user==>id:" << g_currentUser.getId()
       << "name:" << g_currentUser.getName() << endl;
  cout << "-------------------friend list--------------------" << endl;
  //如果好友列表不空就打印一下
  if(!g_currentUserFriendList.empty())
  {
    //friend是c++关键字
    for(User &user : g_currentUserFriendList)
    {
      cout << user.getId() << " " << user.getName() << " " << user.getState()
           << endl;

    }
    cout << "----------------------group list----------------------" << endl;
  }
  if(!g_currentUserGroupList.empty())
  {
    for(Group &group : g_currentUserGroupList)
    {
      cout << group.getId() << " " << group.getName() << " " << group.getDesc()
           << " " << endl;
      for(auto &groupUser : group.getGroupUser())
      {
        cout << groupUser.getId() << " " << groupUser.getName() << " "
             << groupUser.getState() << " " << groupUser.getRole() << endl;
      }
    }
  }
  cout << "===========================================" << endl;
}
//获取系统事件

//涉及到向server发送数据，因此int是接收clientfd的，string是要发送的内容
//help为了和其他函数统一，因此这里使用默认参数，穿的时候可以不用穿
void help(int fd ,string str)
{
  cout<<"show command list"<<endl;
  for(auto &p :commandMap)
  {
    cout << p.first << ":" << p.second << endl;
  }
  cout << endl;
}
void chat(int clientfd,string str)
{
  //现在是查第二个:,相当于借用中间变量，查第二个：
  int idx = str.find(":");
  if(idx==-1)
  {
    cerr << "chat command invaild!" << endl;
    return;
  }
  int friendid = atoi(str.substr(0, idx).c_str());
  string message = str.substr(idx + 1, str.size() - idx);

  json js;
  js["msgid"] = ONE_CAHT_MSG;
  js["id"] = g_currentUser.getId();
  js["name"] = g_currentUser.getName();
  js["toid"] = friendid;
  js["msg"] = message;
  js["time"] = getCurrentTime();
  string buff = js.dump();

  // 向server发送,发送c接口的数据
  int len = send(clientfd, buff.c_str(), sizeof(buff.c_str()) + 1, 0);
  if(len==-1)
  {
    cerr << "send chat msg error->" << buff << endl;
  }
}
void addfriend(int clientfd ,string str)
{
  int friendid = atoi(str.c_str());
  json js;
  js["msgid"] = ADD_FRIEND_MSG;
  js["id"] = g_currentUser.getId();
  js["friendid"] = friendid;
  string buff = js.dump();

  //向server发送,发送c接口的数据
  int len = send(clientfd, buff.c_str(), sizeof(buff.c_str())+1, 0);
  if(len==-1)
  {
    cerr << "send addfriend msg error->" << buff << endl;
  }
}


void creategroup(int clientfd ,string str)
{
  int idx=str.find(":");
  if(idx==-1)
  {
    cerr <<"creategroup command invaild" << endl;
  }
  string groupname = str.substr(0, idx);
  //第二个参数是长度，不是下标
  string groupdesc = str.substr(idx + 1, sizeof(str) - idx);
  json js;
  js["msgid"] = CREATE_GROUP_MSG;
  js["id"] = g_currentUser.getId();
  js["groupname"] = groupname;
  js["groupdesc"] = groupdesc;
  string buff = js.dump();
  

  //向server发送,发送c接口的数据
  int len = send(clientfd, buff.c_str(), sizeof(buff.c_str())+1, 0);
  if(len==-1)
  {
    cerr << "send creategroup msg error->" << buff << endl;
  }
}


void addgroup(int clientfd,string str)
{
  int groupid = atoi(str.c_str());
  json js;
  js["msgid"] = ADD_GROUP_MSG;
  js["id"] = g_currentUser.getId();
  js["groupid"] = groupid;
  string buff = js.dump();

  //向server发送,发送c接口的数据
  int len = send(clientfd, buff.c_str(), sizeof(buff.c_str())+1, 0);
  if(len==-1)
  {
    cerr << "send addgroup msg error->" << buff << endl;
  }
}
void groupchat(int clientfd,string str)
{
  int idx=str.find(":");
  if(idx==-1)
  {
    cerr <<"groupchat command invaild" << endl;
  }
  //atoi处理的是char *
  int groupid = atoi(str.substr(0, idx).c_str());
  //第二个参数是长度，不是下标
  string message = str.substr(idx + 1, sizeof(str) - idx);
  json js;
  js["msgid"] = GROUP_CHAT_MSG;
  js["id"] = g_currentUser.getId();
  js["name"] = g_currentUser.getName();
  js["groupid"] = groupid;
  js["msg"] = message;
  js["time"] = getCurrentTime();
  string buff = js.dump();

  //向server发送,发送c接口的数据
  int len = send(clientfd, buff.c_str(), sizeof(buff.c_str())+1, 0);
  if(len==-1)
  {
    cerr << "send groupchat msg error->" << buff << endl;
  }
}
void quit(int clientfd ,string str)
{
  json js;
  js["msgid"] = LOGINOUT_MSG;
  js["id"] = g_currentUser.getId();
  string buff = js.dump();

  //向server发送,发送c接口的数据
  int len = send(clientfd, buff.c_str(), sizeof(buff.c_str())+1, 0);
  if(len==-1)
  {
    cerr << "send quit msg error->" << buff << endl;
  }
  else{
    isMainMenuRunning = false;
  }
}

void mainMenu(int clientfd)
{
  help();
  char buff[1024] = {0};
  while(isMainMenuRunning)
  {
    cin.getline(buff, 1024);
    string commandbuf(buff);
    string command;
    int idx = commandbuf.find(":");//自定义的命令中，以：间隔
    if(-1==idx)
    {
      command = commandbuf;//有的命令没有：
    }
    else
    {
      command = commandbuf.substr(0, idx);
    }
    auto it = commandHandlerMap.find(command);
    
    if(it == commandHandlerMap.end())
    {
      cerr << "invalid input command" << endl;
      continue;
    }

    //调用对应的命令处理回调
    it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
  }
} 

string getCurrentTime()
{
   // 获取当前时间的秒数
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    time(&rawtime);  // 获取当前的秒数

    // 将秒数转换为本地时间
    timeinfo = localtime(&rawtime);

    // 格式化时间为字符串
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return buffer;
}
