#include "EasyTcpServer.hpp"

#include <cstdio>
#include <thread>    //c++标准线程库

bool g_bRun = true;
void cmdThread(EasyTcpServer *server) {
	while (true) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			g_bRun = false;
			server->Close();
			printf("client exit cmdThread...\n");
			break;
		}
		else {
			printf("no support cmd.\n");
		}
	}
}

int main() {
	EasyTcpServer server;
	server.InitSocket();
	server.Bind(nullptr, 4567);
	server.Listen(5);

	//启动UI线程
	std::thread t1(cmdThread, &server);
	//与主线程分离
	t1.detach();
	
	while (g_bRun) {
		server.OnRun();
	}
	server.Close();

	printf("exit.\n");
	getchar();
	return 0;
}