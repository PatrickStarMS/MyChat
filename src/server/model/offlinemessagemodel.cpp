#include "offlinemessagemodel.hpp"
#include "ConnectionPool.h"

void OfflineMsgModel::insert(int userid, std::string msg)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage value(%d,'%s')", userid, msg.c_str());
    
    auto conn = ConnectionPool::GetConnectionPool().GetConnection();
    conn->Update(sql);
}

//返回给用户之后，就删除给该用户的离线消息
void OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d", userid);
    
    auto conn = ConnectionPool::GetConnectionPool().GetConnection();
    conn->Update(sql);
}


//查询用户的离线消息，离线消息可能不止一个。这里使用vector容器存储,可以与js直接转化
vector<string> OfflineMsgModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);
    
    vector<string> vec;
    auto conn = ConnectionPool::GetConnectionPool().GetConnection();
    MYSQL_RES* res = conn->Query(sql);
    
    if (res != nullptr) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr) {
            vec.push_back(row[0]);
        }
        mysql_free_result(res);
    }
    return vec;
}