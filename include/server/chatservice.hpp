#ifndef __CHATSERVICE_HPP__
#define __CHATSERVICE_HPP__

#include "groupmodel.hpp"
#include "muduo/net/TcpConnection.h"
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include <unordered_map>
#include <functional>
#include <mutex>
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

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 服务器异常退出重置方法
    void reset();

    // 获取消息对应的处理方法
    MsgHandler getHandler(int msgid);
private:
    // 消息id与对应处理方法的映射表
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 构造函数私有化
    ChatService();

    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // _userConnMap的互斥锁
    mutex _connMutex;
    
    // 数据操作类实例
    // 用户
    UserModel _userModel;
    // 离线消息
    OfflineMsgModel _offlineMsgModel;
    // 好友关系
    FriendModel _friendModel;
    // 群组管理
    GroupModel _groupModel;
};

#endif