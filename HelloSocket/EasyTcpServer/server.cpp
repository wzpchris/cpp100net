#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <WinSock2.h>  //这里会产生宏重复定义问题,需要添加宏定义WIN32_LEAN_AND_MEAN
#include <cstdio>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

enum CMD {
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_ERROR,
};

//消息头
struct DataHeader {
	short dataLength;
	short cmd;
};

//DataPackage
struct Login: public DataHeader {
	Login() {
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char UserName[32];
	char PassWord[32];
};

struct LoginResult:public DataHeader {
	LoginResult() {
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 1;
	}
	int result;
};

struct LogOut:public DataHeader {
	LogOut() {
		dataLength = sizeof(LogOut);
		cmd = CMD_LOGOUT;
	}
	char UserName[32];
};

struct LogOutResult:public DataHeader {
	LogOutResult() {
		dataLength = sizeof(LogOutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 1;
	}
	int result;
};

std::vector<SOCKET> g_clients;

int processor(SOCKET _cSock) {
	//缓冲区处理粘包与分包问题
	char szRecv[1024] = {};
	//5.接收客户端数据
	int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0);
	DataHeader *header = (DataHeader*)szRecv;
	if (nLen <= 0) {
		printf("client exit\n");
		return -1;
	}

	/*if (nLen >= header->dataLength) {

	}*/
	//6.处理请求
	switch (header->cmd)
	{
		case CMD_LOGIN:
		{
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			Login *login = (Login*)szRecv;
			printf("recv client msg: [len=%d, cmd=%d, username=%s, pwd=%s]\n", login->dataLength, login->cmd, login->UserName, login->PassWord);
			//忽略判断用户密码是否正确的过程
			LoginResult ret;
			send(_cSock, (const char*)&ret, sizeof(LoginResult), 0);
		}
		break;
		case CMD_LOGOUT:
		{
			recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
			LogOut *logout = (LogOut*)szRecv;
			printf("recv client msg: [len=%d, cmd=%d, username=%s]\n", logout->dataLength, logout->cmd, logout->UserName);
			//忽略判断用户密码是否正确的过程
			LogOutResult ret;
			send(_cSock, (const char*)&ret, sizeof(LogOutResult), 0);
		}
		break;
		default:
		{
			header->cmd = CMD_ERROR;
			header->dataLength = 0;
			send(_cSock, (const char*)&header, sizeof(DataHeader), 0);
		}
		break;
	};

	return 0;
}

int main() {
	//启动Windows socket 2.X环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	//启动，需要添加库文件ws2_32.lib
	WSAStartup(ver, &dat);
	//----------------
	//用Scoket API建立简易TCP服务端
	//1.建立一个Socket 套接字
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//2.bind绑定用户接收客户端链接的网络端口
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	_sin.sin_addr.S_un.S_addr = INADDR_ANY;  //inet_addr("127.0.0.1");
	if (SOCKET_ERROR == bind(_sock, (const sockaddr *)&_sin, sizeof(sockaddr_in))) {
		printf("bind error...\n");
	}
	else {
		printf("bind success...\n");
	}
	//3.listen监听端口
	if (SOCKET_ERROR == listen(_sock, 5))
	{
		printf("listen error...\n");
	}
	else {
		printf("listen success...\n");
	}

	while (true) {
		//伯克利 socket
		fd_set fdRead;
		fd_set fdWrite;
		fd_set fdExp;
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		FD_SET(_sock, &fdRead);
		FD_SET(_sock, &fdWrite);
		FD_SET(_sock, &fdExp);

		for (auto _fd : g_clients) {
			FD_SET(_fd, &fdRead);
		}

		//nfds是一个整数值，是指fd_set集合中所有描述符(socket)的范围，而不是数量
		//即是所有文件描述符最大值+1，在windows中这个参数可以写0
		timeval t = { 0, 0 };
		int ret = select(_sock + 1, &fdRead, &fdWrite, &fdExp, &t);
		if (ret < 0) {
			printf("select error exit\n");
			break;
		}

		if (FD_ISSET(_sock, &fdRead)) {
			FD_CLR(_sock, &fdRead);

			//4.accept等待接收客户端链接
			sockaddr_in clientAddr = {};
			int nAddrLen = sizeof(clientAddr);
			SOCKET _cSock = INVALID_SOCKET;

			_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
			if (INVALID_SOCKET == _cSock) {
				printf("error invalid SOCKET...\n");
			}

			g_clients.push_back(_cSock);
			printf("new client: socket =%d, IP = %s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));
		}
	
		for (size_t n = 0; n < fdRead.fd_count; ++n) {
			if (-1 == processor(fdRead.fd_array[n])) {
				auto iter = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[n]);
				if (iter != g_clients.end()) {
					g_clients.erase(iter);
				}
			}
		}
	}

	for (auto s : g_clients) {
		closesocket(s);
	}
	
	//8.关闭套接字closesocket
	closesocket(_sock);
	//-------------------
	//关闭
	WSACleanup();
	printf("exit.\n");
	getchar();
	return 0;
}