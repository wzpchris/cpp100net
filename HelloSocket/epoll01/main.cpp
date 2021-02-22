#include <unistd.h>	
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <thread>

#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)

#include <cstdio>
#include <vector>
#include <algorithm>

std::vector<SOCKET> g_clients;

bool g_bRun = true;
void cmdThread() {
    while(true) {
        char cmdBuf[256] = {};
        scanf("%s", cmdBuf);
        if(0 == strcmp(cmdBuf, "exit")) {
            g_bRun = false;
            printf("退出cmdThread线程\n");
            break;
        }else {
            printf("不支持的命令.\n");
        }
    }
}

int cell_epoll_ctl(int epfd, int op, SOCKET sockfd, uint32_t events) {
    epoll_event ev;
    //事件类型
    ev.events = events;
    //事件关联的socket描述对象
    ev.data.fd = sockfd;
    // 向epoll对象注册需要管理，监听的socket文件描述符
    // 并且说明关心的事件
    // 返回0代表操作陈宫，返回负值戴斌失败-1
    if(epoll_ctl(epfd, op, sockfd, &ev) == -1) {
        if(events & EPOLLIN) {
            printf("error, epoll_ctl(%d, %d, %d, %d)\n", epfd, op, sockfd, EPOLLIN);
        }

        if(events & EPOLLOUT) {
            printf("error, epoll_ctl(%d, %d, %d, %d)\n", epfd, op, sockfd, EPOLLOUT);
        }

        if(events & EPOLLERR) {
            printf("error, epoll_ctl(%d, %d, %d, %d)\n", epfd, op, sockfd, EPOLLERR);
        }

        if(events & EPOLLHUP) {
            printf("error, epoll_ctl(%d, %d, %d, %d)\n", epfd, op, sockfd, EPOLLHUP);
        }
        
    }
}

char g_szBuff[4096] = {};
int g_nLen;
int readData(SOCKET cSock) {
    g_nLen = (int)recv(cSock, g_szBuff, sizeof(g_szBuff), 0);
    return g_nLen;
}

int writeData(SOCKET cSock) {
    if(g_nLen > 0) {
        int nLen = (int)send(cSock, g_szBuff, g_nLen, 0);
        g_nLen = 0;
        return nLen;
    }
    
    return 0;
}

int clientLeave(SOCKET sock) {
    printf("客户端sock=%d已退出,任务结束.\n", sock);
    close(sock);
    auto iter = std::find(g_clients.begin(), g_clients.end(), sock);
    g_clients.erase(iter);
}

int main() {
    //启动cmd线程
    std::thread t1(cmdThread);
    t1.detach();

    //--用socket api 建立简易tcp服务端
    // 1 建立一个socket 套接字
    SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // 2 bind 绑定用于接收客户端连接的网络端口
    sockaddr_in _sin;
    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(4567);
    _sin.sin_addr.s_addr = INADDR_ANY;
    if(SOCKET_ERROR == bind(_sock, (const sockaddr*)&_sin, sizeof(sockaddr_in))) {
        printf("error, 绑定网络端口失败...\n");
    }else {
        printf("绑定网络端口成功...\n");
    }

    // 3 listen 监听网络端口
    if(SOCKET_ERROR == listen(_sock, 5)) {
        printf("error, 监听网络端口失败...\n");
    }else {
        printf("监听网络端口成功...\n");
    }

    // linux 2.6.8后size就没有什么作用了
    // 由epoll动态管理，理论最大值为filemax
    // 通过cat /proc/sys/fs/file-max来查询
    int epfd = epoll_create(256);
    cell_epoll_ctl(epfd, EPOLL_CTL_ADD, _sock, EPOLLIN);
    
    //
    int msgCount = 0;
    //用于接收检测到的网络事件的数据
    epoll_event events[256] = {};
    while(g_bRun) {
        //epfd epoll对象的描述符
        //events 用于接收检测到的网络事件的数组
        //maxevents 接收数组的大小,能够接收的事件的数量
        //timeout 
        //  t=-1:直到有事件发生才返回
        //  t=0:立即返回
        //  t>0:等待指定数值毫秒后返回
        int n = epoll_wait(epfd, events, 256, 0);
        if(n < 0) {
            printf("epoll_wait ret %d\n", n);
            break;
        }

        for(int i = 0; i < n; ++i) {
            //与服务的socket相同，有新客户连接
            if(events[i].data.fd == _sock) {
                sockaddr_in clientAddr = {};
                int nAddrLen = sizeof(sockaddr_in);
                SOCKET _cSock = INVALID_SOCKET;
                _cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
                if(INVALID_SOCKET == _cSock) {
                    printf("error, 接收到无效客户端socket...\n");
                }else {
                    g_clients.push_back(_cSock);
                    cell_epoll_ctl(epfd, EPOLL_CTL_ADD, _cSock, EPOLLIN);
					// TODO LT and ET 视频312中有提到这里的LT的效果
                    // cell_epoll_ctl(epfd, EPOLL_CTL_ADD, _cSock, EPOLLIN | EPOLLOUT);
                    // cell_epoll_ctl(epfd, EPOLL_CTL_ADD, _cSock, EPOLLIN | EPOLLOUT | EPOLLLT);
                    // cell_epoll_ctl(epfd, EPOLL_CTL_ADD, _cSock, EPOLLIN | EPOLLOUT | EPOLLET);
                    // cell_epoll_ctl(epfd, EPOLL_CTL_ADD, _cSock, EPOLLIN | EPOLLET);
                    printf("新客户端加入 socket=%d, IP=%s\n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
                }   
                continue; 
            }
            //当前socket有数据可读,也有可能发生错误
            if(events[i].events & EPOLLIN) {
                printf("EPOLLIN | %d\n", ++msgCount);
                auto cSockfd = events[i].data.fd;
                int ret = readData(cSockfd);
                if(ret < 0) {
                    clientLeave(cSockfd);
                }else {
                    printf("接收客户端数据 id = %d, socket=%d, len=%d\n", msgCount, cSockfd, ret);
                } 
                cell_epoll_ctl(epfd, EPOLL_CTL_MOD, cSockfd, EPOLLOUT);
            }
            //当前socket有数据可写,也有可能发生错误
            if(events[i].events & EPOLLOUT) {
                printf("EPOLLOUT | %d\n", msgCount);
                auto cSockfd = events[i].data.fd;
                int ret = writeData(cSockfd);
                if(ret < 0) {
                    clientLeave(cSockfd);
                }

                cell_epoll_ctl(epfd, EPOLL_CTL_MOD, cSockfd, EPOLLIN);
            }
            
            /* if(events[i].events & EPOLLERR) {
                auto cSockfd = events[i].data.fd;
                printf("EPOLLERR id=%d, socket=%d\n", msgCount, cSockfd);
            }
            
            if(events[i].events & EPOLLHUP) {
                auto cSockfd = events[i].data.fd;
                printf("EPOLLHUP id=%d, socket=%d\n", msgCount, cSockfd);
            } */
        }        
    }

    for(auto client : g_clients) {
        close(client);
    }
    close(_sock);
    close(epfd);
    printf("已退出\n");
    return 0;
}
