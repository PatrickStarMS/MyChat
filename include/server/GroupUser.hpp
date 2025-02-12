#ifndef GROUPUSER_H
#define GROUPUSER
#include "user.hpp"
#include <string>
using namespace std;
class GroupUser : public User {
public:
  void setRole(string role) { _role = role; }
  string getRole() { return _role; }
private:
  //这里是继承不是组合，因此不用使用User作成员变量
 string _role;
};
#endif