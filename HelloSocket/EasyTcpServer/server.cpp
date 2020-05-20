#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <WinSock2.h>  //这里会产生宏重复定义问题,需要添加宏定义WIN32_LEAN_AND_MEAN
#include <cstdio>

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
		printf("ERROR,绑定端口失败...\n");
	}
	else {
		printf("绑定端口成功...\n");
	}
	//3.listen监听端口
	if (SOCKET_ERROR == listen(_sock, 5))
	{
		printf("监听网络端口失败...\n");
	}
	else {
		printf("监听网络端口成功...\n");
	}

	//4.accept等待接收客户端链接
	sockaddr_in clientAddr = {};
	int nAddrLen = sizeof(clientAddr);
	SOCKET _cSock = INVALID_SOCKET;

	_cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
	if (INVALID_SOCKET == _cSock) {
		printf("错误，接收无效客户端SOCKET...\n");
	}
	printf("新客户端加入:socket =%d, IP = %s \n", (int)_cSock, inet_ntoa(clientAddr.sin_addr));

	
	while (true) {
		DataHeader header = {};
		//5.接收客户端数据
		int nLen = recv(_cSock, (char *)&header, sizeof(DataHeader), 0);
		if (nLen <= 0) {
			printf("客户端已退出，任务结束.");
			break;
		}

		//6.处理请求
		switch (header.cmd) 
		{
			case CMD_LOGIN:
			{
				Login login = {};
				recv(_cSock, (char*)&login + sizeof(DataHeader), sizeof(Login) - sizeof(DataHeader), 0);
				printf("recv client msg: [len=%d, cmd=%d, username=%s, pwd=%s]\n", login.dataLength, login.cmd, login.UserName, login.PassWord);
				//忽略判断用户密码是否正确的过程
				LoginResult ret;
				send(_cSock, (const char*)&ret, sizeof(LoginResult), 0);
			}
			break;
			case CMD_LOGOUT: 
			{
				LogOut logout = {};
				recv(_cSock, (char*)&logout + sizeof(DataHeader), sizeof(LogOut) - sizeof(DataHeader), 0);
				printf("recv client msg: [len=%d, cmd=%d, username=%s]\n", logout.dataLength, logout.cmd, logout.UserName);
				//忽略判断用户密码是否正确的过程
				LogOutResult ret;
				send(_cSock, (const char*)&ret, sizeof(LogOutResult), 0);
			}
			break;
			default: 
			{
				header.cmd = CMD_ERROR;
				header.dataLength = 0;
				send(_cSock, (const char*)&header, sizeof(DataHeader), 0);
			}
			break;
		};
	}
	
	//8.关闭套接字closesocket
	closesocket(_cSock);
	//-------------------
	//关闭
	WSACleanup();
	printf("已退出，任务结束.");
	getchar();
	return 0;
}