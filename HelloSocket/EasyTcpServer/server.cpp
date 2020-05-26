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

class MyServer:public EasyTcpServer
{
public:
	MyServer() {}
	~MyServer() {}
	//客户端加入事件
	virtual void OnNetJoin(ClientSocket *pClient) {
		EasyTcpServer::OnNetJoin(pClient);
	}
	//客户端离开事件
	virtual void OnNetLeave(ClientSocket *pClient) {
		EasyTcpServer::OnNetLeave(pClient);
	}
	//客户端消息事件
	virtual void OnNetMsg(CellServer *pCellServer, ClientSocket *pClient, DataHeader *header) {
		EasyTcpServer::OnNetMsg(pCellServer, pClient, header);
		//6.处理请求
		switch (header->cmd)
		{
			case CMD_LOGIN:
			{
				Login *login = (Login*)header;
				//printf("recv client msg: [len=%d, cmd=%d, username=%s, pwd=%s]\n", login->dataLength, login->cmd, login->UserName, login->PassWord);
				//忽略判断用户密码是否正确的过程
				//发送数据 这里的发送有性能瓶颈
				//接收 消息  -->  处理 发送
				//生产者  数据缓冲区  消费者
				/*LoginResult ret;
				pClient->SendData(&ret);*/
				LoginResult *ret = new LoginResult();	
				pCellServer->addSendTask(pClient, ret);
			}
			break;
			case CMD_LOGOUT:
			{
				LogOut *logout = (LogOut*)header;
				//printf("recv client msg: [len=%d, cmd=%d, username=%s]\n", logout->dataLength, logout->cmd, logout->UserName);
				//忽略判断用户密码是否正确的过程
				/*LogOutResult ret;
				pClient->SendData(&ret);*/
				LogOutResult *ret = new LogOutResult();
				pCellServer->addSendTask(pClient, ret);
			}
			break;
			default:
			{
				printf("recv unknow sock=%d, msglen=%d...\n", pClient->sockfd(), header->dataLength);
				/*DataHeader ret;
				pClient->SendData(&ret);*/
			}
			break;
		};
	}

	virtual void OnNetRecv(ClientSocket *pClient) {
		EasyTcpServer::OnNetRecv(pClient);
	}
};

int main() {
	MyServer server;
	server.InitSocket();
	server.Bind(nullptr, 4567);
	server.Listen(5);
	server.Start(4);

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