#include "friendmodel.hpp"
#include "ConnectionPool.h"

//所有的insert都差不多，稍微修改一下就行
void FriendModel::insert(int userid, int friendid)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into friend value(%d,%d)", userid, friendid);
    
    auto conn = ConnectionPool::GetConnectionPool().GetConnection();
    conn->Update(sql);
}

vector<User> FriendModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on \
            b.friendid=a.id where b.userid = %d", userid);
    
    vector<User> vec;
    auto conn = ConnectionPool::GetConnectionPool().GetConnection();
    MYSQL_RES* res = conn->Query(sql);
    
    if (res != nullptr) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr) {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setState(row[2]);
            vec.push_back(user);
        }
        mysql_free_result(res);
    }
    return vec;
}
