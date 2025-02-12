#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

//  匹配USER表的ORM类
class User {
public:
 User(int id_ =-1,string name_="",string pwd_="",string state_="offline"):id(id_)
 ,name(name_),
 passward(pwd_),
 state(state_)
 {}
 void setId(int id) { this->id = id; }
 void setName(string name) { this->name = name; }
 void setPassword(string passward) { this->passward=passward; }
 void setState(string state) { this->state = state; }

 int getId() { return id; }
 string getName() { return name; }
 string getPassword() { return passward; }
 string getState() { return state; }   

protected : 
 int id;
 string name;
 string passward;
 string state;
};

#endif