#ifndef _EASY_TCP_SERVER_HPP_
#define _EASY_TCP_SERVER_HPP_

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <windows.h>
	#include <WinSock2.h>  //这里会产生宏重复定义问题,需要添加宏定义WIN32_LEAN_AND_MEAN
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>	
	#include <arpa/inet.h>
	#include <string.h>

	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR (-1)
#endif

#include <cstdio>
#include <vector>
#include "MessageHeader.hpp"

class EasyTcpServer {
private:
	SOCKET _sock;
	std::vector<SOCKET> g_clients;
public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
	}

	virtual ~EasyTcpServer() {
		Close();
	}

	//初始化socket
	SOCKET InitSocket() {
		//启动Win Sock 2.x环境
#ifdef _WIN32
		//启动Windows socket 2.X环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		//启动，需要添加库文件ws2_32.lib
		WSAStartup(ver, &dat);
#endif
		//1.建立一个socket
		if (INVALID_SOCKET != _sock) {
			printf("close before socket...\n");
			Close();
		}

		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			printf("socket error ...\n");
		}
		else {
			printf("socket success sock=%d...\n", (int)_sock);
		}
		return _sock;
	}
	//绑定端口号
	int Bind(const char *ip, unsigned int port) {
		if (INVALID_SOCKET == _sock) {
			InitSocket();
		}
		//2.bind绑定用户接收客户端链接的网络端口
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
		
#ifdef _WIN32
		if (ip != NULL) {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else 
		if (ip != NULL) {
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (const sockaddr *)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret) {
			printf("bind error...\n");
		}
		else {
			printf("bind success port[%d]...\n", (int)port);
		}

		return ret;
	}

	//监听端口号
	int Listen(int n) {
		//3.listen监听端口
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			printf("listen error sock[%d]...\n", (int)_sock);
		}
		else {
			printf("listen success sock[%d]...\n", (int)_sock);
		}

		return ret;
	}

	//接收客户端连接
	SOCKET Accpet() {
		//4.accept等待接收客户端链接
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(clientAddr);
		SOCKET _cSock = INVALID_SOCKET;
#ifdef _WIN32
		_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		_cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
		if (INVALID_SOCKET == _cSock) {
			printf("error invalid SOCKET sock[%d]...\n", (int)_sock);
		}
		else {
			NewUserJoin userJoin;
			userJoin.sock = _cSock;
			SendDataToAll(&userJoin);

			g_clients.push_back(_cSock);
			printf("new client: socket =%d, IP = %s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
		}

		return _cSock;
	}

	//关闭socket
	void Close() {
		if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
			for (auto s : g_clients) {
				closesocket(s);
			}

			//8.关闭套接字closesocket
			closesocket(_sock);
			WSACleanup();
#else 
			for (auto s : g_clients) {
				close(s);
			}
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
		}
	}
	//处理网络消息
	bool OnRun() {
		if (IsRun()) {
			//伯克利 BSD	socket
			fd_set fdRead;
			fd_set fdWrite;
			fd_set fdExp;
			FD_ZERO(&fdRead);
			FD_ZERO(&fdWrite);
			FD_ZERO(&fdExp);

			FD_SET(_sock, &fdRead);
			FD_SET(_sock, &fdWrite);
			FD_SET(_sock, &fdExp);

			SOCKET maxSock = _sock;
			for (auto _fd : g_clients) {
				FD_SET(_fd, &fdRead);
				if (_fd > maxSock) {
					maxSock = _fd;
				}
			}

			//nfds是一个整数值，是指fd_set集合中所有描述符(socket)的范围，而不是数量
			//即是所有文件描述符最大值+1，在windows中这个参数可以写0
			timeval t = { 0, 0 }; //这是是非阻塞，将导致单核CPU达到100%
			int ret = select(maxSock + 1, &fdRead, &fdWrite, &fdExp, &t);
			if (ret < 0) {
				printf("select error exit\n");
				Close();
				return false;
			}

			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);
				Accpet();
			}

			for (int n = (int)g_clients.size() - 1; n >= 0; --n) {
				if (FD_ISSET(g_clients[n], &fdRead)) {
					if (-1 == RecvData(g_clients[n])) {
						auto iter = g_clients.begin() + n;
						if (iter != g_clients.end()) {
							g_clients.erase(iter);
						}
					}
				}
			}

			return true;
		}
		return false;
	}
	//是否工作中
	bool IsRun() {
		return _sock != INVALID_SOCKET;
	}
	//接收数据 处理粘包 拆分包
	int RecvData(SOCKET cSock) {
		//缓冲区处理粘包与分包问题
		char szRecv[1024] = {};
		//5.接收客户端数据
		int nLen = (int)recv(cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader *header = (DataHeader*)szRecv;
		if (nLen <= 0) {
			printf("client socket=%d exit\n", cSock);
			return -1;
		}

		/*if (nLen >= header->dataLength) {

		}*/
		recv(cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(cSock, header);
		return 0;
	}
	//响应网络消息
	virtual void OnNetMsg(SOCKET cSock, DataHeader *header) {
		//6.处理请求
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			Login *login = (Login*)header;
			printf("recv client msg: [len=%d, cmd=%d, username=%s, pwd=%s]\n", login->dataLength, login->cmd, login->UserName, login->PassWord);
			//忽略判断用户密码是否正确的过程
			LoginResult ret;
			SendData(cSock, &ret);
		}
		break;
		case CMD_LOGOUT:
		{
			LogOut *logout = (LogOut*)header;
			printf("recv client msg: [len=%d, cmd=%d, username=%s]\n", logout->dataLength, logout->cmd, logout->UserName);
			//忽略判断用户密码是否正确的过程
			LogOutResult ret;
			SendData(cSock, &ret);
		}
		break;
		default:
		{
			header->cmd = CMD_ERROR;
			header->dataLength = 0;
			SendData(cSock, header);
		}
		break;
		};
	}
	//发送指定Socket数据
	int SendData(SOCKET cSock, DataHeader *header) {
		if (IsRun() && header) {
			return send(cSock, (const char *)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
	//发送指定Socket数据
	void SendDataToAll(DataHeader *header) {
		if (IsRun() && header) {
			for (auto s : g_clients) {
				SendData(s, header);
			}
		}
	}
};

#endif