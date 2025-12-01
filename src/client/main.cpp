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
        perror("Failed to create socket");
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