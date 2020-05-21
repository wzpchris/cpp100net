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
#include <thread>    //c++标准线程库



enum CMD {
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_ERROR,
};

//消息头
struct DataHeader {
	short dataLength;
	short cmd;
};

//DataPackage
struct Login : public DataHeader {
	Login() {
		dataLength = sizeof(Login);
		cmd = CMD_LOGIN;
	}
	char UserName[32];
	char PassWord[32];
};

struct LoginResult :public DataHeader {
	LoginResult() {
		dataLength = sizeof(LoginResult);
		cmd = CMD_LOGIN_RESULT;
		result = 1;
	}
	int result;
};

struct LogOut :public DataHeader {
	LogOut() {
		dataLength = sizeof(LogOut);
		cmd = CMD_LOGOUT;
	}
	char UserName[32];
};

struct LogOutResult :public DataHeader {
	LogOutResult() {
		dataLength = sizeof(LogOutResult);
		cmd = CMD_LOGOUT_RESULT;
		result = 1;
	}
	int result;
};

struct NewUserJoin :public DataHeader {
	NewUserJoin() {
		dataLength = sizeof(NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};

int processor(SOCKET _cSock) {
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
	//6.处理请求
	switch (header->cmd)
	{
	case CMD_LOGIN_RESULT:
	{
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LoginResult *loginret = (LoginResult*)szRecv;
		printf("recv server CMD_LOGIN_RESULT msg: [len=%d, cmd=%d, result=%d]\n",
			loginret->dataLength, loginret->cmd, loginret->result);
	}
	break;
	case CMD_LOGOUT_RESULT:
	{
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		LogOutResult *logoutret = (LogOutResult*)szRecv;
		printf("recv server CMD_LOGOUT_RESULT msg: [len=%d, cmd=%d, result=%d]\n",
			logoutret->dataLength, logoutret->cmd, logoutret->result);
	}
	break;
	case CMD_NEW_USER_JOIN:
	{
		recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0);
		NewUserJoin *newJoin = (NewUserJoin*)szRecv;
		printf("recv server CMD_NEW_USER_JOIN msg: [len=%d, cmd=%d, sock=%d]\n",
			newJoin->dataLength, newJoin->cmd, newJoin->sock);
	}
	break;
	};

	return 0;
}

bool g_bRun = true;
void cmdThread(SOCKET sock) {
	while (true) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			g_bRun = false;
			printf("client exit cmdThread...\n");
			break;
		}
		else if (0 == strcmp(cmdBuf, "login")) {
			Login login;
			strcpy(login.UserName, "tom");
			strcpy(login.PassWord, "tom");
			send(sock, (const char *)&login, sizeof(Login), 0);
		}
		else if (0 == strcmp(cmdBuf, "logout")) {
			LogOut logout;
			strcpy(logout.UserName, "tom");
			send(sock, (const char *)&logout, sizeof(LogOut), 0);
		}
		else {
			printf("no support cmd.\n");
		}
	}
}

int main() {
#ifdef _WIN32
	//启动Windows socket 2.X环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	//启动，需要添加库文件ws2_32.lib
	WSAStartup(ver, &dat);
#endif
	//-----------------------
	//1.建立一个socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock) {
		printf("socket error ...\n");
	}
	else {
		printf("socket success ...\n");
	}
	//2.链接服务器connect
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
#ifdef _WIN32
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
#else 
	_sin.sin_addr.s_addr = inet_addr("192.168.1.101");
#endif
	int ret = connect(_sock, (const sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret) {
		printf("connect error ...\n");
	}
	else {
		printf("connect success ...\n");
	}

	//启动线程
	std::thread t1(cmdThread, _sock);
	//与主线程分离
	t1.detach();

	while (g_bRun) {
		fd_set fdReads;
		FD_ZERO(&fdReads);

		FD_SET(_sock, &fdReads);

		timeval t = { 0, 0 };
		int ret = select(_sock + 1, &fdReads, NULL, NULL, &t);
		if (ret < 0) {
			printf("select error ...\n");
			break;
		}

		if (FD_ISSET(_sock, &fdReads)) {
			FD_CLR(_sock, &fdReads);

			if (-1 == processor(_sock)) {
				printf("server no msg\n");
				break;
			}
		}
	}

#ifdef _WIN32
	//7.关闭套接字closesocket
	closesocket(_sock);
	//-----------------------

	//关闭
	WSACleanup();
#else 
	close(_sock);
#endif

	printf("exit.\n");
	getchar();
	return 0;
}