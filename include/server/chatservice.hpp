#ifndef __CHATSERVICE_HPP__
#define __CHATSERVICE_HPP__

#include "muduo/net/TcpConnection.h"
#include "json.hpp"
#include "usermodel.hpp"
#include <unordered_map>
#include <functional>
using namespace muduo;
using namespace muduo::net;
using namespace std;

using json = nlohmann::json;

// 处理消息的事件回调方法类型
using MsgHandler = function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

// 聊天服务类
class ChatService
{
public:
    // 单例构造接口
    static ChatService* instance();

    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 获取消息对应的处理方法
    MsgHandler getHandler(int msgid);
private:
    // 消息id与对应处理方法的映射表
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 构造函数私有化
    ChatService();
    
    // 数据操作类
    UserModel _userModel;
};


#endif