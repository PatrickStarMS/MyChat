#ifndef OFFLINEMODEL_H
#define OFFLINEMODEL_H
#include <string>
#include <vector>
using namespace std;
// 提供离线消息表的离线消息
class OfflineMsgModel
{
public:
    //存储用户的离线消息,服务器与数据库不是通过js，而是直接send字节流
 void insert(int userid, std::string msg);

 //返回给用户之后，就删除给该用户的离线消息
 void remove(int userid);
 

 //查询用户的离线消息，离线消息可能不止一个。这里使用vector容器存储,可以与js直接转化
 vector<string> query(int userid);
};
#endif