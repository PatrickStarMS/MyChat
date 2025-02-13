#include <iostream>
#include "chatserver.hpp"
using namespace std;
#include <signal.h>
#include "chatservice.hpp"
#include "ConnectionPool.h"
//处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int){

    chatservice::getChatInstance()->reset();
    exit(0);
}
int main(int argc,char **argv)
{
  if(argc<3)
  {
    cerr << "ChatServer 127.0.0.1 6000 或者ChatServer 127.0.0.1 6002" << endl;
    exit(-1);
  }
  //ip需要字符串（c形式的） port需要整型
  char* ip = argv[1];
  uint16_t port = atoi(argv[2]);

  signal(SIGINT, resetHandler);

  EventLoop loop;
  InetAddress addr(ip,port);

  

  chatserver chat(&loop, addr, "chatserver");
  chat.start();
  loop.loop();
  return 0;
}
