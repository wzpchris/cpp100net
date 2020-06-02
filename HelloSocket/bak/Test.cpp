#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>  //这里会产生宏重复定义问题,需要添加宏定义WIN32_LEAN_AND_MEAN

//#pragma comment(lib, "ws2_32.lib")

int main() {
	//启动Windows socket 2.X环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	//启动，需要添加库文件ws2_32.lib
	WSAStartup(ver, &dat);
	//TODO:
	//关闭
	WSACleanup();

	return 0;
}