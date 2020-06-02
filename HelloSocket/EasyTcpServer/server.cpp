//#include "Alloctor.h"
#include "EasyTcpServer.hpp"
#include "CellMsgStream.hpp"

#include <cstdio>
#include <thread>    //c++标准线程库
//
//bool g_bRun = true;
//void cmdThread(EasyTcpServer *server) {
//	
//}

class MyServer:public EasyTcpServer
{
public:
	MyServer() {}
	~MyServer() {}
	//客户端加入事件
	virtual void OnNetJoin(CellClient *pClient) {
		EasyTcpServer::OnNetJoin(pClient);
	}
	//客户端离开事件
	virtual void OnNetLeave(CellClient *pClient) {
		EasyTcpServer::OnNetLeave(pClient);
	}
	//客户端消息事件
	virtual void OnNetMsg(CellServer *pCellServer, CellClient *pClient, netmsg_DataHeader *header) {
		EasyTcpServer::OnNetMsg(pCellServer, pClient, header);
		//6.处理请求
		switch (header->cmd)
		{
			case CMD_LOGIN:
			{
				pClient->resetDtHeart();
				netmsg_Login *login = (netmsg_Login*)header;
				//CellLog::Info("recv client msg: [len=%d, cmd=%d, username=%s, pwd=%s]\n", login->dataLength, login->cmd, login->UserName, login->PassWord);
				//忽略判断用户密码是否正确的过程
				//发送数据 这里的发送有性能瓶颈
				//接收 消息  -->  处理 发送
				//生产者  数据缓冲区  消费者
				netmsg_LoginR ret;
				if (SOCKET_ERROR == pClient->SendData(&ret)) {
					//发送缓冲区满了，消息没发出去
					//这里可以将没有发送出去的消息放入数据库或者文件中，之后再发送
					CellLog::Info("send buff full sock[%d]\n", pClient->sockfd());
				}
				/*netmsg_LoginR *ret = new netmsg_LoginR();	
				pCellServer->addSendTask(pClient, ret);*/
			}
			break;
			case CMD_LOGOUT:
			{
				//netmsg_LogOut *logout = (netmsg_LogOut*)header;
				////CellLog::Info("recv client msg: [len=%d, cmd=%d, username=%s]\n", logout->dataLength, logout->cmd, logout->UserName);
				////忽略判断用户密码是否正确的过程
				//netmsg_LogOutR ret;
				//pClient->SendData(&ret);
				/*netmsg_LogOutR *ret = new netmsg_LogOutR();
				pCellServer->addSendTask(pClient, ret);*/
				
				CellRecvStream r(header);

				auto n1 = r.readInt8();
				auto n2 = r.readInt16();
				auto n3 = r.readInt32();
				auto n4 = r.readFloat();
				auto n5 = r.readDouble();

				uint32_t n = 0;
				r.onlyRead(n);

				char name[32] = {};
				auto n6 = r.readArray(name, 32);
				char pw[32] = {};
				auto n7 = r.readArray(pw, 32);

				int data[10] = {};
				auto n8 = r.readArray(data, 10);

				CellSendStream s;
				s.setNetCmd(CMD_LOGOUT_RESULT);
				s.writeInt8(1);
				s.writeInt16(2);
				s.writeInt32(3);
				s.writeFloat(4.5f);
				s.writeDouble(6.7);
				const char* str = "server";
				s.writeArray(str, strlen(str));
				char a[] = "ahah";
				s.writeArray(a, strlen(a));
				int b[] = { 1, 2, 3, 4, 5 };
				s.writeArray(b, 5);
				s.finish();

				pClient->SendData(s.data(), s.length());
			}
			break;
			case CMD_C2S_HEART:
			{
				pClient->resetDtHeart();
				netmsg_s2c_Heart ret;
				pClient->SendData(&ret);
			}
			break;
			default:
			{
				CellLog::Info("recv unknow sock=%d, msglen=%d...\n", pClient->sockfd(), header->dataLength);
				/*netmsg_DataHeader ret;
				pClient->SendData(&ret);*/
			}
			break;
		};
	}

	virtual void OnNetRecv(CellClient *pClient) {
		EasyTcpServer::OnNetRecv(pClient);
	}
};

int main() {
	CellLog::Instance().setLogPath("serverLog.txt", "w");
	MyServer server;
	server.InitSocket();
	server.Bind(nullptr, 4567);
	server.Listen(64);
	server.Start(4);

	////启动UI线程
	//std::thread t1(cmdThread, &server);
	////与主线程分离
	//t1.detach();
	
	while (true) {
		char cmdBuf[256] = {};
		scanf("%s", cmdBuf);
		if (0 == strcmp(cmdBuf, "exit")) {
			server.Close();
			CellLog::Info("client exit cmdThread...\n");
			//CellLog::Info("client exit cmdThread...\n");
			break;
		}
		else {
			CellLog::Info("no support cmd.\n");
		}
	}
	
	CellLog::Info("exit.\n");
	/*CellTaskServer task;
	task.Start();
	Sleep(100);
	task.Close();*/
//#ifdef _WIN32
//	while (true) {
//		Sleep(1);
//	}
//#endif
	return 0;
}