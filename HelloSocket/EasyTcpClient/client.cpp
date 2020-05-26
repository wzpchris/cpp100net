#include "EasyTcpClient.hpp"
#include "CellTimestamp.hpp"

#include <cstdio>
#include <thread>    //c++标准线程库
#include <atomic>

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
const int nCount = 10000;
//发送线程数量
const int tCount = 4;
//这里不能是EasyTcpClient的数组，因为栈的大小大约只有1M,这里会爆栈
EasyTcpClient* client[nCount];
std::atomic_int sendCount = 0;
std::atomic_int readyCount = 0;

void sendThread(int id) {  //4个线程 ID 1-4
	printf("thread<%d>, start\n", id);
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
	}

	printf("thread<%d>, Connect<begin=%d, end=%d>\n", id, begin, end);
	
	readyCount++;
	while (readyCount < tCount) { //等待其他线程准备好发送数据
		std::chrono::milliseconds t(3000);
		std::this_thread::sleep_for(t);
	}

	Login login[1];
	for (int n = 0; n < 1; n++) {
		strcpy(login[n].UserName, "tom");
		strcpy(login[n].PassWord, "tom");
	}
	
	const int nLen = sizeof(login);
	while (g_bRun) {
		for (int n = begin; n < end; ++n) {
			if (SOCKET_ERROR != client[n]->SendData(login, nLen)) {
				sendCount++;
			}
			//client[n]->OnRun();
		}
	}

	for (int n = begin; n < end; ++n) {
		client[n]->Close();
		delete client[n];
	}
	printf("thread<%d>, exit\n", id);
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

	CellTimestamp tTime;
	while (g_bRun) {
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0) {
			printf("thread<%d>,clients<%d>,time<%lf>,send<%d>\n", tCount, nCount, t, (int)sendCount);
			sendCount = 0;
			tTime.update();
		}
		Sleep(1);
	}

	printf("exit.\n");
	return 0;
}