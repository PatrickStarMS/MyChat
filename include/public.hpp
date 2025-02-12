#ifndef PUBLIC
#define PUBLIC
//一个消息类型对应一个处理函数，用于回调区分
enum EnMsgType {
  LOGIN_MSG = 1,
  LOGIN_MSG_ACK,
  REG_MSG,
  REG_MSG_ACK,     // 注册响应信息
  ONE_CAHT_MSG,    // 一对一聊天消息
  ADD_FRIEND_MSG,  // 添加好友信息

  CREATE_GROUP_MSG,
  ADD_GROUP_MSG,
  GROUP_CHAT_MSG,

  LOGINOUT_MSG,//注销消息
};

#endif