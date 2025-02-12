#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;

class GroupModel
{
public:
//创建群组
 bool createGroup(Group &group);
//加入群组
 void addGroup(int userid, int groupid, string role);
 //查询用户所在群组信息
 vector<Group> queryGroup(int userid);
 //根据指定的groupid查询用户id列表，除自己的userid之外，主要用户群聊业务给其他成员发消息
 vector<int> queryGroupUser(int userid,int groupid);
};

#endif // !GROUPMODEL_H
