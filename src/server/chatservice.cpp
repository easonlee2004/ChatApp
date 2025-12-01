#include "chatservice.hpp"
#include "public.hpp"
#include "muduo/base/Logging.h"
#include "usermodel.hpp"
#include "friendmodel.hpp"
#include <functional>
#include <mutex>
#include <vector>
using namespace muduo;
using namespace muduo::net;

// 获取单例对象的函数
ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 记录消息和对应回调操作
ChatService::ChatService()
{
    // 用户业务回调
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务回调
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
}

// 获取消息对应的处理方法
MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // 返回默认处理
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) {
            LOG_ERROR << "msgid: " << msgid << " Cannot find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务 
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];
    
    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 用户已登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "This account is already logged in; please use a different account.";
            conn->send(response.dump());
        }
        else
        {
            // 登陆成功，用map记录用户连接信息
            // 加锁保证map的线程安全
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // 修改state
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 检查是否有离线消息
            vector<string> vec = _offlineMsgModel.query(user.getId());
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 离线消息存入vector后应该将其删除
                _offlineMsgModel.remove(id);
            }

            // 查询登录用户的好友及信息
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                // vec2用于存储好友信息js
                // string和js等价
                // 相当于response中再嵌套一层friends js，要反序列化两次
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户的群组信息
            vector<Group> groupVec = _groupModel.queryGroups(id);
            if (!groupVec.empty())
            {
                vector<string> groupV; // groupVec存对象，groupV存json
                for (Group &group : groupVec)
                {
                    json gjs;
                    gjs["id"] = group.getId();
                    gjs["groupname"] = group.getName();
                    gjs["groupdesc"] = group.getDesc();
                    
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json ujs;
                        ujs["id"] = user.getId();
                        ujs["name"] = user.getName();
                        ujs["state"] = user.getState();
                        ujs["role"] = user.getRole();
                        userV.push_back(ujs.dump());
                    }
                    gjs["users"] = userV;
                    groupV.push_back(gjs.dump());
                }
                response["groups"] = groupV;
            }
            conn->send(response.dump());
        }
    }
    else
    {
        // 用户不存在，登陆失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "Username or password is invalid.";
        conn->send(response.dump());
    }
}

// 处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 反序列化
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["id"] = user.getId();
        response["errno"] = 0;
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                // 从map中删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 更新用户状态为offline
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 服务器异常退出重置方法
void ChatService::reset()
{
    // 把online的用户设置为offline
    _userModel.resetState();
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["to"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，直接发送
            it->second->send(js.dump());
            return;
        }
    }

    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> vec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for (int id : vec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            // 存储离线消息
            _offlineMsgModel.insert(id, js.dump());
        }
    }
}