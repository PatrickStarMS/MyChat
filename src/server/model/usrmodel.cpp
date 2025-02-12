#include "usermodel.hpp"
#include "db.h"
#include "ConnectionPool.h"
#include <iostream>
//因为使传入整个用用户，所以传入user（下面查虚函数，只需要传入id就行了，把查到的所有信息存到user中
bool UserModel::insert(User& user)
{
    char sql[1024] = {0};
    sprintf(sql,
            "insert into user(name,passward,state) value('%s', '%s','%s')",
            user.getName().c_str(), user.getPassword().c_str(),
            user.getState().c_str());
    
    auto conn = ConnectionPool::GetConnectionPool().GetConnection();
    if (conn->Update(sql)) {
        user.setId(mysql_insert_id(conn->GetConnect()));
        return true;
    }
    return false;
}

User UserModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);
    
    auto conn = ConnectionPool::GetConnectionPool().GetConnection();
    MYSQL_RES* res = conn->Query(sql);
    if (res != nullptr) {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr) {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setPassword(row[2]);
            user.setState(row[3]);
            mysql_free_result(res);
            return user;
        }
    }
    return User();
}

bool UserModel::updateState(User& user)
{
    char sql[1024] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d",
            user.getState().c_str(), user.getId());
    
    auto conn = ConnectionPool::GetConnectionPool().GetConnection();
    return conn->Update(sql);
}

//和上面的差不多一样，不过这个是更新所有的用户状态
void UserModel::resetState()
{
    char sql[1024] = {0};
    sprintf(sql, "update user set state = 'offline' where state = 'online'");
    
    auto conn = ConnectionPool::GetConnectionPool().GetConnection();
    conn->Update(sql);
}