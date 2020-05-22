#ifndef _EASY_TCP_CLIENT_HPP_
#define _EASY_TCP_CLIENT_HPP_

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
#include <stdio.h>
#include "MessageHeader.hpp"


class EasyTcpClient
{
	SOCKET _sock;
public:
	EasyTcpClient() {
		_sock = INVALID_SOCKET;
	}

	virtual ~EasyTcpClient() {
		Close();
	}

	//初始化socket
	void InitSocket() {
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
			printf("socket success sock=%d...\n", _sock);
		}
	}

	//连接服务器
	int Connect(const char *ip, short port) {
		if (INVALID_SOCKET == _sock) {
			InitSocket();
		}
		//2.链接服务器connect
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else 
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		int ret = connect(_sock, (const sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret) {
			printf("connect error sock=%d ip=%s:%d...\n", _sock, ip, port);
		}
		else {
			printf("connect success sock=%d ip=%s:%d...\n", _sock, ip, port);
		}

		return ret;
	}

	//关闭socket
	void Close() {
		//关闭Win Sock 2.x环境
		if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
			//7.关闭套接字closesocket
			closesocket(_sock);
			WSACleanup();
#else 
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
		}
	}

	//处理网络消息
	bool OnRun() {
		if (IsRun()) {
			fd_set fdReads;
			FD_ZERO(&fdReads);

			FD_SET(_sock, &fdReads);

			timeval t = { 0, 0 };
			int ret = select(_sock + 1, &fdReads, NULL, NULL, &t);
			if (ret < 0) {
				printf("select error socket[%d]...\n", _sock);
				Close();
				return false;
			}

			if (FD_ISSET(_sock, &fdReads)) {
				FD_CLR(_sock, &fdReads);

				if (-1 == RecvData(_sock)) {
					printf("server no msg socket[%d]\n", _sock);
					Close();
					return false;
				}
			}

			return true;
		}
		
		return false;
	}

	//是否工作
	bool IsRun() {
		return _sock != INVALID_SOCKET;
	}

	//接收数据 处理粘包 拆分包
	int RecvData(SOCKET _cSock) {
		//缓冲区处理粘包与分包问题
		char szRecv[1024] = {};
		//5.接收客户端数据
		int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
		DataHeader *header = (DataHeader*)szRecv;
		if (nLen <= 0) {
			printf("connection break socket=%d exit\n", _cSock);
			return -1;
		}

		/*if (nLen >= header->dataLength) {

		}*/

		recv(_sock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		OnNetMsg(header);
		return 0;
	}

	//响应网络消息
	virtual void OnNetMsg(DataHeader *header) {
		//6.处理请求
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			LoginResult *loginret = (LoginResult*)header;
			printf("recv server CMD_LOGIN_RESULT msg: [len=%d, cmd=%d, result=%d]\n",
				loginret->dataLength, loginret->cmd, loginret->result);
		}
		break;
		case CMD_LOGOUT_RESULT:
		{
			LogOutResult *logoutret = (LogOutResult*)header;
			printf("recv server CMD_LOGOUT_RESULT msg: [len=%d, cmd=%d, result=%d]\n",
				logoutret->dataLength, logoutret->cmd, logoutret->result);
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			NewUserJoin *newJoin = (NewUserJoin*)header;
			printf("recv server CMD_NEW_USER_JOIN msg: [len=%d, cmd=%d, sock=%d]\n",
				newJoin->dataLength, newJoin->cmd, newJoin->sock);
		}
		break;
		};
	}

	//发送数据
	int SendData(DataHeader *header) {
		if (IsRun() && header) {
			return send(_sock, (const char *)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
private:

};

#endif 
