#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <WinSock2.h>  //这里会产生宏重复定义问题,需要添加宏定义WIN32_LEAN_AND_MEAN
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

struct DataPackage {
	int age;
	char name[32];
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

	char _recvBuf[128] = {};

	while (true) {
		//5.接收客户端数据
		int nLen = recv(_cSock, _recvBuf, 128, 0);
		if (nLen <= 0) {
			printf("客户端已退出，任务结束.");
			break;
		}

		printf("recv client msg: %s\n", _recvBuf);

		//6.处理请求
		if (0 == strcmp(_recvBuf, "getInfo")) {
			DataPackage dp = { 80, "Tom" };
			//7.send向客户端发送一条数据
			send(_cSock, (const char*)&dp, sizeof(DataPackage), 0);
		}
		else {
			char msgBuf[] = "???.";
			//7.send向客户端发送一条数据
			send(_cSock, msgBuf, strlen(msgBuf) + 1, 0);
		}
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