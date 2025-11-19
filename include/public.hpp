#ifndef __PUBLIC_HPP__
#define __PUBLIC_HPP__

/*
server和client的公共文件
*/

enum EnMsgType
{
    LOGIN_MSG = 1, // 登录消息
    REG_MSG, // 注册消息
    REG_MSG_ACK  // 注册响应消息
};

#endif