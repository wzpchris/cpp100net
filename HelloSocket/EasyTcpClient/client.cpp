#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <windows.h>
#include <WinSock2.h>  //这里会产生宏重复定义问题,需要添加宏定义WIN32_LEAN_AND_MEAN
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

enum CMD {
	CMD_LOGIN,
	CMD_LOGOUT,
	CMD_ERROR,
};

//消息头
struct DataHeader {
	short dataLength;
	short cmd;
};

//DataPackage
struct Login {
	char UserName[32];
	char PassWord[32];
};

struct LoginResult {
	int result;
};

struct LogOut {
	char UserName[32];
};

struct LogOutResult {
	int result;
};

int main() {
	//启动Windows socket 2.X环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	//启动，需要添加库文件ws2_32.lib
	WSAStartup(ver, &dat);
	//-----------------------
	//1.建立一个socket
	SOCKET _sock = socket(AF_INET, SOCK_STREAM, 0);
	if (INVALID_SOCKET == _sock) {
		printf("错误,建立socket失败...\n");
	}
	else {
		printf("建立socket成功...\n");
	}
	//2.链接服务器connect
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	_sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int ret = connect(_sock, (const sockaddr*)&_sin, sizeof(sockaddr_in));
	if (SOCKET_ERROR == ret) {
		printf("错误,连接服务器失败...\n");
	}
	else {
		printf("连接服务器成功...\n");
	}
	
	while (true) {
		//3.输入请求命令
		char cmdBuf[128] = {};
		scanf("%s", cmdBuf);

		//4.处理请求
		if (0 == strcmp(cmdBuf, "exit")) {
			printf("接收到退出命令exit\n");
			break;
		}
		else if (0 == strcmp(cmdBuf, "login")) {
			Login login = { "tom", "tom" };
			DataHeader header = {};
			header.dataLength = sizeof(Login);
			header.cmd = CMD_LOGIN;
			//5.向服务器发送请求
			send(_sock, (const char*)&header, sizeof(DataHeader), 0);
			send(_sock, (const char*)&login, sizeof(Login), 0);

			//接收服务器返回的数据
			DataHeader recvHeader = {};
			LoginResult loginRet = {};
			recv(_sock, (char*)&recvHeader, sizeof(DataHeader), 0);
			recv(_sock, (char*)&loginRet, sizeof(LoginResult), 0);

			printf("LoginResult: %d\n", loginRet.result);
		}
		else if (0 == strcmp(cmdBuf, "logout")) {
			LogOut logout = { "tom" };
			DataHeader header = {};
			header.dataLength = sizeof(LogOut);
			header.cmd = CMD_LOGOUT;
			//5.向服务器发送请求
			send(_sock, (const char*)&header, sizeof(DataHeader), 0);
			send(_sock, (const char*)&logout, sizeof(LogOut), 0);

			//接收服务器返回的数据
			DataHeader recvHeader = {};
			LogOutResult logoutRet = {};
			recv(_sock, (char*)&recvHeader, sizeof(DataHeader), 0);
			recv(_sock, (char*)&logoutRet, sizeof(LogOutResult), 0);

			printf("LogOutResult: %d\n", logoutRet.result);
		}
		else {
			printf("no support cmd.\n");
		}
	}
	
	//7.关闭套接字closesocket
	closesocket(_sock);
	//-----------------------
	//关闭
	WSACleanup();

	printf("已退出，任务结束.");
	getchar();
	return 0;
}