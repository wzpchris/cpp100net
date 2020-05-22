#include "EasyTcpClient.hpp"

#include <cstdio>
#include <thread>    //c++标准线程库

void cmdThread(EasyTcpClient *client) {
	while (true) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			client->Close();
			printf("client exit cmdThread...\n");
			break;
		}
		else if (0 == strcmp(cmdBuf, "login")) {
			Login login;
			strcpy(login.UserName, "tom");
			strcpy(login.PassWord, "tom");
			client->SendData(&login);
		}
		else if (0 == strcmp(cmdBuf, "logout")) {
			LogOut logout;
			strcpy(logout.UserName, "tom");
			client->SendData(&logout);
		}
		else {
			printf("no support cmd.\n");
		}
	}
}

int main() {
	EasyTcpClient client;
	client.InitSocket();
	client.Connect("127.0.0.1", 4567);

	//启动UI线程
	std::thread t1(cmdThread, &client);
	//与主线程分离
	t1.detach();

	while (client.IsRun()) {
		client.OnRun();
	}

	client.Close();
	printf("exit.\n");
	getchar();
	return 0;
}