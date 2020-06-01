#include "EasyTcpClient.hpp"
#include "CellTimestamp.hpp"

#include <cstdio>
#include <thread>    //c++标准线程库
#include <atomic>

class MyClient :public EasyTcpClient {
public:
	virtual void OnNetMsg(netmsg_DataHeader *header) {
		//6.处理请求
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			netmsg_LoginR *loginret = (netmsg_LoginR*)header;
			/*CellLog::Info("recv server CMD_LOGIN_RESULT msg: [len=%d, cmd=%d, result=%d]\n",
				loginret->dataLength, loginret->cmd, loginret->result);*/
		}
		break;
		case CMD_LOGOUT_RESULT:
		{
			netmsg_LogOutR *logoutret = (netmsg_LogOutR*)header;
			/*CellLog::Info("recv server CMD_LOGOUT_RESULT msg: [len=%d, cmd=%d, result=%d]\n",
				logoutret->dataLength, logoutret->cmd, logoutret->result);*/
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			netmsg_NewUserJoin *newJoin = (netmsg_NewUserJoin*)header;
			/*CellLog::Info("recv server CMD_NEW_USER_JOIN msg: [len=%d, cmd=%d, sock=%d]\n",
				newJoin->dataLength, newJoin->cmd, newJoin->sock);*/
		}
		break;
		case CMD_ERROR:
		{
			CellLog::Info("recv error msg len=%d...\n", header->dataLength);
		}
		break;
		default: {
			CellLog::Info("recv unkown msglen=%d...\n", header->dataLength);
		}
		};
	}
};

bool g_bRun = true;
void cmdThread() {
	while (true) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			g_bRun = false;
			CellLog::Info("client exit cmdThread...\n");
			break;
		}
		else {
			CellLog::Info("no support cmd.\n");
		}
	}
}

//客户端数量
const int nCount = 1000;
//发送线程数量
const int tCount = 4;
//这里不能是EasyTcpClient的数组，因为栈的大小大约只有1M,这里会爆栈
EasyTcpClient* client[nCount];
std::atomic_int sendCount = 0;
std::atomic_int readyCount = 0;

void recvThread(int begin, int end) {
	CellTimestamp t;
	while (g_bRun) {
		for (int n = begin; n < end; ++n) {
			if (t.getElapsedSecond() > 3.0 && n == begin) {
				continue;
			}
			client[n]->OnRun();
		}
	}
}

void sendThread(int id) {  //4个线程 ID 1-4
	CellLog::Info("thread<%d>, start\n", id);
	int c = nCount / tCount;
	int begin = (id - 1) * c;
	int end = id * c;
	for (int n = begin; n < end; ++n) {
		if (!g_bRun) {
			return;
		}
		client[n] = new MyClient();
	}

	for (int n = begin; n < end; ++n) {
		if (!g_bRun) {
			return;
		}
		client[n]->InitSocket();
		client[n]->Connect("127.0.0.1", 4567);
	}

	CellLog::Info("thread<%d>, Connect<begin=%d, end=%d>\n", id, begin, end);
	
	readyCount++;
	while (readyCount < tCount) { //等待其他线程准备好发送数据
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}

	//启动接收线程
	std::thread t1(recvThread, begin, end);
	t1.detach();
	//

	netmsg_Login login[10];
	for (int n = 0; n < 10; n++) {
		strcpy(login[n].UserName, "tom");
		strcpy(login[n].PassWord, "tom");
	}
	
	const int nLen = sizeof(login);
	while (g_bRun) {
		for (int n = begin; n < end; ++n) {
			if (SOCKET_ERROR != client[n]->SendData(login)) {
				sendCount++;
			}
		}
		std::chrono::milliseconds t(100);
		std::this_thread::sleep_for(t);
	}
	
	for (int n = begin; n < end; ++n) {
		client[n]->Close();
		delete client[n];
	}
	CellLog::Info("thread<%d>, exit\n", id);
}

int main() {
	CellLog::Instance().setLogPath("clientLog.log", "w");
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
			CellLog::Info("thread<%d>,clients<%d>,time<%lf>,send<%d>\n", tCount, nCount, t, (int)(sendCount / t));
			sendCount = 0;
			tTime.update();
		}
		Sleep(1);
	}

	CellLog::Info("exit.\n");
	return 0;
}