#ifndef USERMODEL_H
#define USERMODE_H

#include "user.hpp"
//user表的数据操作类
class UserModel{
public:
  //增删改查，调用数据库的增删改查，然后使数据库数据和ORM进行交互
 bool insert(User &user);
 //查询
 User query(int id);
 //这里传user或者state都行
 bool updateState(User &user);
 //reset the state of user
 void resetState();
};

#endif