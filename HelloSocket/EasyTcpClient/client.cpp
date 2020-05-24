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

//客户端数量
const int nCount = 1000;
//发送线程数量
const int tCount = 4;
//这里不能是EasyTcpClient的数组，因为栈的大小大约只有1M,这里会爆栈
EasyTcpClient* client[nCount];

void sendThread(int id) {  //4个线程 ID 1-4
	int c = nCount / tCount;
	int begin = (id - 1) * c;
	int end = id * c;
	for (int n = begin; n < end; ++n) {
		if (!g_bRun) {
			return;
		}
		client[n] = new EasyTcpClient();
	}

	for (int n = begin; n < end; ++n) {
		if (!g_bRun) {
			return;
		}
		client[n]->InitSocket();
		client[n]->Connect("127.0.0.1", 4567);
		printf("Connect=%d\n", n);
	}

	Login login;
	strcpy(login.UserName, "tom");
	strcpy(login.PassWord, "tom");
	while (g_bRun) {
		//client.OnRun();
		//client.SendData(&login);
		for (int n = begin; n < end; ++n) {
			client[n]->SendData(&login);
			//client[n]->OnRun();
		}
	}

	for (int n = begin; n < end; ++n) {
		client[n]->Close();
	}
}

int main() {
	//启动UI线程
	std::thread t1(cmdThread);
	//与主线程分离
	t1.detach();

	for (int n = 0; n < tCount; ++n) {
		//启动发送线程
		std::thread t1(sendThread, n + 1);
		t1.detach();
	}

	while (g_bRun) {
		Sleep(100);
	}

	printf("exit.\n");
	return 0;
}