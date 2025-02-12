#include "db.h"

//初始化，看成员变量和成员方法
//相当于给它分配一块内存
MySQL::MySQL() { _conn = mysql_init(nullptr); }
//断开连接（释放数据库资源）
MySQL::~MySQL() { mysql_close(_conn); }

//连接数据库
bool MySQL::connect()
{
  MYSQL *p =
      mysql_real_connect(_conn, server.c_str(), user.c_str(), password.c_str(),
                         dbname.c_str(), 3306, nullptr, 0);
      if(p!=nullptr)
      {
        //从数据库拿到的数据不支持中文，这里设置一下
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success!";
      }
      else{
        LOG_INFO << "connect mysql fail!";
      }
      return p;
}

//更新操作
bool MySQL::update(string sql){
  
  if (mysql_query(_conn, sql.c_str())) {
    LOG_INFO << __FILE__ << ":" << __LINE__ << ":" << sql << "更新失败!"
     << " 错误信息: " << mysql_error(_conn);;
    return false;
  }
        return true;
}

//查询操作
MYSQL_RES* MySQL::query(string sql)
    {
        if (mysql_query(_conn, sql.c_str()))
        {
            LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                << sql << "查询失败!";
            return nullptr;
        }
        return mysql_use_result(_conn);
    }

    MYSQL* MySQL::getConnection() { return _conn;}