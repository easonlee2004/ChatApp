#include "json.hpp"
#include "group.hpp"
#include "user.hpp"
#include "public.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
using namespace std;
using json = nlohmann::json;

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <semaphore.h>
#include <atomic>

// 记录当前用户信息
User g_currentUser;

// 当前用户好友列表
vector<User> g_currentUserFriendList;

// 当前好友所在群组
vector<Group> g_currentUserGroupList;

// 显示当前用户信息
void showCurrentUserData();

// 多线程接收函数句柄
void readTaskHandler(int clientfd);

// 主聊天页面
void mainMenu();

// 主页面command对应的函数句柄
void help(int fd = 0, string str = "");
void chat(int, string);
void addfriend(int, string);
void addgroup(int, string);
void creategroup(int, string);
void groupchat(int, string);
void logout(int, string);

// 支持的合法命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"logout", "注销，格式logout"}
};

// 
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"logout", logout}
};


int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "Command invalid! Example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // main函数传入的ip和port
    char *ip = argv[1];
    unsigned short port = atoi(argv[2]);

    // socket
    struct sockaddr_in servaddr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Failed to create socket!");
        return -1;
    }

    // connect
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip);
    servaddr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
    {
        perror("Connection failed!");
        close(sockfd);
        return -1;
    }

    while (1)
    {
        // 选择界面
        cout << "========================" << endl;
        cout << "1. LOGIN" << endl;
        cout << "2. REGISTER" << endl;
        cout << "3. QUIT" << endl;
        cout << "========================" << endl;
        cout << "Please input: ";
        int selection;
        cin >> selection;
        cin.get();

        switch(selection)
        {
            case 1: // 登陆业务
            {
                int id = 0;
                char password[1024] = {0};
                cout << "Input your id: ";
                cin >> id;
                cin.get();
                cout << "Input password: ";
                cin.getline(password, 1024);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = password;
                string request = js.dump();

                int len = send(sockfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if (len == -1)
                {
                    cerr << "Send login message error: " << request << endl;
                }

                // 分离线程用于接收ack信息
                thread readTask(readTaskHandler, sockfd);
                readTask.detach();

                mainMenu();
            }
            break;
            case 2: // 注册业务
            {
                char name[1024] = {0};
                char password[1024] = {0};
                cout << "Input your name: ";
                cin.getline(name, 1024);
                cout << "Input your password: ";
                cin.getline(password, 1024);

                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = password;
                string request = js.dump();

                int len = send(sockfd, request.c_str(), strlen(request.c_str()) + 1, 0);
                if (len == -1)
                {
                    cerr << "Send register message error: " << request << endl;
                }
            }
            break;
            case 3: // 退出客户端
            {
                close(sockfd);
                exit(0);
            }
            default:
            {
                cerr << "Invalid input!" << endl;
                break;
            }
        }
    }

    return 0;
}

// 显示当前用户信息
void showCurrentUserData()
{
    cout << "==================== Current User ====================" << endl;
    cout << "Current user ID: " << g_currentUser.getId() << ", name: " << g_currentUser.getName() << endl;
    cout << "-------------------- Friend List --------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "-------------------- Group List --------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << "   " << user.getId() << " " << user.getName() << " " << user.getState() << " " << user.getRole() << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}


// 多线程接收函数句柄
void readTaskHandler(int clientfd)
{
    while (1)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (len == 0 || len == -1)
        {
            close(clientfd);
            exit(1);
        }

        json js = json::parse(buffer);

        if (js["msgid"] == ONE_CHAT_MSG)
        {

        }

        if (js["msgid"] == GROUP_CHAT_MSG)
        {

        }

        if (js["msgid"] == LOGIN_MSG_ACK)
        {
            if (js["errno"].get<int>() != 0)
            {
                cerr << js["errmsg"] << endl;

            }
            else
            {
                g_currentUser.setId(js["id"].get<int>());
                g_currentUser.setName(js["name"]);
            }

        }

        if (js["msgid"] == REG_MSG_ACK)
        {

        }
    }

}

// 主聊天页面
void mainMenu()
{
    
}

void help(int, string)
{

}

void chat(int fd, string str)
{

}

void addfriend(int fd, string str)
{

}

void addgroup(int fd, string str)
{

}

void creategroup(int fd, string str)
{

}

void groupchat(int fd, string str)
{

}

void logout(int fd, string str)
{

}