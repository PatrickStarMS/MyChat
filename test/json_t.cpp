#include "../include/json.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <map>
using json = nlohmann::json;

//json序列化
void serialize() { 
    json js;
    js["msg_type"] = 2;
    js["from"] = "张三";
    js["to"] = "lisi";
    js["message"] = "hello";
    std::string str = js.dump();
    std::cout << js << std::endl;
    std::cout << str.c_str() << std::endl;
}
//json序列2，值随意
void serialize2() { 
    json js;
    // 添加数组
    js["id"] = {1,2,3,4,5}; 
    // 添加key-value
    js["name"] = "zhang san"; 
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china"; 
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang  san", "hello world"}, {"liu shuo", "hello china"}};
    std::cout << js << std::endl;
    
}
//容器序列化,直接赋值即可
std::string serialize3() { 
    json js;
    // 直接序列化一个vector容器
    std::vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);
    js["list"] = vec;
    // 直接序列化一个map容器
    std::map<int, std::string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});
    js["path"] = m;
    
    std::cout << js << std::endl;
    return js.dump();
    
}

//反序列化，也是直接赋值
void parse3(std::string str)
{
    //形式要对应
    // 模拟从网络接收到json字符串，通过json::parse函数把json字符串专程json对象
    json js2 = json::parse(str);
    
    // 直接取key-value
    //形式一定要对应，serialize3里面没有name，因此下面就会报错
    // std::string name = js2["name"];
    // std::cout << __LINE__ << std::endl;
    // std::cout << "name:" << name << std::endl;
    // 直接反序列化vector容器
   std::vector<int> v = js2["list"];
    for(int val : v)
    {
    std::cout << val << " ";
    }
    std::cout << std::endl;
    // 直接反序列化map容器
    std::map<int, std::string> m2 = js2["path"];
    for(auto p : m2)
    {
    std::cout << p.first << " " << p.second << std::endl;
    }
    std::cout << std::endl;
}


int main(int argc, const char** argv) {
  serialize();
  serialize2();
  auto str = serialize3();
//   std::cout << str << std::endl;
  parse3(str);

  std::cout << "hello json" << std::endl;
  return 0;
}