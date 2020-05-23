#include "EasyTcpClient.hpp"

#include <cstdio>
#include <thread>    //c++标准线程库

bool g_bRun = true;
void cmdThread() {
	while (true) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			g_bRun = false;
			printf("client exit cmdThread...\n");
			break;
		}
		else {
			printf("no support cmd.\n");
		}
	}
}

int main() {
	const int nCount = FD_SETSIZE - 1;
	//这里不能是EasyTcpClient的数组，因为栈的大小大约只有1M,这里会爆栈
	EasyTcpClient* client[nCount];
	for (int n = 0; n < nCount; ++n) {
		client[n] = new EasyTcpClient();
		client[n]->InitSocket();
		client[n]->Connect("127.0.0.1", 4567);
	}

	//启动UI线程
	std::thread t1(cmdThread);
	//与主线程分离
	t1.detach();

	Login login;
	strcpy(login.UserName, "tom");
	strcpy(login.PassWord, "tom");
	while (g_bRun) {
		//client.OnRun();
		//client.SendData(&login);
		for (int n = 0; n < nCount; ++n) {
			client[n]->SendData(&login);
			client[n]->OnRun();
		}
	}

	for (int n = 0; n < nCount; ++n) {
		client[n]->Close();
	}
	printf("exit.\n");
	getchar();
	return 0;
}