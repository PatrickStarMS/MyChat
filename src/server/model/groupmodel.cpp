#include "groupmodel.hpp"
#include "chatservice.hpp"
#include "ConnectionPool.h"
using namespace std;
// 创建群组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname,groupdesc) value('%s','%s')",
            group.getName().c_str(), group.getDesc().c_str());
    
    auto conn = ConnectionPool::GetConnectionPool().GetConnection();
    if (conn->Update(sql)) {
        group.setId(mysql_insert_id(conn->GetConnect()));
        return true;
    }
    return false;
}
//加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser(userid,groupid,grouprole) value(%d,%d,'%s')",
            userid, groupid, role.c_str());
    
    auto conn = ConnectionPool::GetConnectionPool().GetConnection();
    conn->Update(sql);
}
//查询用户所在群组信息,除了返回群组id之外还要返回群组的信息（因此联合查表）
vector<Group> GroupModel::queryGroup(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join \
            groupuser b on a.id=b.groupid where b.userid = %d", userid);
    
    vector<Group> groupVec;
    auto conn = ConnectionPool::GetConnectionPool().GetConnection();
    MYSQL_RES* res = conn->Query(sql);
    
    if (res != nullptr) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr) {
            Group group;
            group.setId(atoi(row[0]));
            group.setName(row[1]);
            group.setDesc(row[2]);
            groupVec.push_back(group);
        }
        mysql_free_result(res);
    }

    // 查询组员
    for (Group &group : groupVec) {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join \
                groupuser b on a.id=b.userid where b.groupid=%d", group.getId());
        
        auto conn = ConnectionPool::GetConnectionPool().GetConnection();
        MYSQL_RES* res = conn->Query(sql);
        
        if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getGroupUser().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}
//根据指定的groupid查询用户id列表，除自己的userid之外，主要用户群聊业务给其他成员发消息
vector<int> GroupModel::queryGroupUser(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid=%d and userid !=%d",
            groupid, userid);
    
    vector<int> idVec;
    auto conn = ConnectionPool::GetConnectionPool().GetConnection();
    MYSQL_RES* res = conn->Query(sql);
    
    if (res != nullptr) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr) {
            idVec.push_back(atoi(row[0]));
        }
        mysql_free_result(res);
    }
    return idVec;
}