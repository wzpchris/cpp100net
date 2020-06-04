//#include "Alloctor.h"
#include "CellLog.hpp"
#include "EasyTcpServer.hpp"
#include "CellMsgStream.hpp"
#include "CellConfig.hpp"

#include <cstdio>
#include <thread>    //c++标准线程库
//
//bool g_bRun = true;
//void cmdThread(EasyTcpServer *server) {
//	
//}

class MyServer :public EasyTcpServer
{
public:
	MyServer() {}
	~MyServer() {}
	/// <summary>
	/// 客户端加入事件
	/// </summary>
	/// <param name="pClient"></param>
	virtual void OnNetJoin(CellClient* pClient) {
		EasyTcpServer::OnNetJoin(pClient);
	}
	//客户端离开事件
	virtual void OnNetLeave(CellClient* pClient) {
		EasyTcpServer::OnNetLeave(pClient);
	}
	/// <summary>
	/// 客户端消息事件
	/// </summary>
	/// <param name="pCellServer"></param>
	/// <param name="pClient"></param>
	/// <param name="header"></param>
	virtual void OnNetMsg(CellServer* pCellServer, CellClient* pClient, netmsg_DataHeader* header) {
		EasyTcpServer::OnNetMsg(pCellServer, pClient, header);
		//6.处理请求
		switch (header->cmd)
		{
		case CMD_LOGIN:
		{
			pClient->resetDtHeart();
			netmsg_Login* login = (netmsg_Login*)header;
			/// <summary>
			/// //CellLog::Info("recv client msg: [len=%d, cmd=%d, username=%s, pwd=%s]\n", login->dataLength, login->cmd, login->UserName, login->PassWord);
			//忽略判断用户密码是否正确的过程
			//发送数据 这里的发送有性能瓶颈
			//接收 消息  -->  处理 发送
			//生产者  数据缓冲区  消费者
			/// </summary>
			/// <param name="pCellServer"></param>
			/// <param name="pClient"></param>
			/// <param name="header"></param>
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

			CellReadStream r(header);

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

			CellWriteStream s;
			s.setNetCmd(CMD_LOGOUT_RESULT);
			s.writeInt8(n1);
			s.writeInt16(n2);
			s.writeInt32(n3);
			s.writeFloat(n4);
			s.writeDouble(n5);
			s.writeArray(name, n6);
			s.writeArray(pw, n7);
			s.writeArray(data, n8);
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

	virtual void OnNetRecv(CellClient* pClient) {
		EasyTcpServer::OnNetRecv(pClient);
	}
};

char* argToStr(int argc, char* args[], int index, char* def, const char* argName) {
	if (index >= argc) {
		CellLog_Error("argToStr, index=%d argc=%d argName=%s\n", index, argc, argName);
	}
	else {
		def = args[index];
	}

	CellLog_Info("%s=%s\n", argName, def);
	return def;
}

int argToInt(int argc, char* args[], int index, int def, const char* argName) {
	if (index >= argc) {
		CellLog_Error("argToStr, index=%d argc=%d argName=%s\n", index, argc, argName);
	}
	else {
		def = atoi(args[index]);
	}

	CellLog_Info("%s=%d\n", argName, def);
	return def;
}

int main(int argc, char* args[]) {
	CellLog::Instance().setLogPath("serverLog.log", "w");
	
	CellConfig::Instance().Init(argc, args);

	/*const char* strIPDef = "127.0.0.1";
	char* strIP = argToStr(argc, args, 1, (char*)strIPDef, "strIP");
	uint16_t nPort = argToInt(argc, args, 2, 4567, "nPort");
	int nThread = argToInt(argc, args, 3, 1, "nThread");
	int nClient = argToInt(argc, args, 4, 1, "nClient");*/
	const char* strIP = CellConfig::Instance().getStr("strIP", "127.0.0.1");
	uint16_t nPort = CellConfig::Instance().getInt("nPort", 4567);
	int nThread = CellConfig::Instance().getInt("nThread", 1);
	int nClient = CellConfig::Instance().getInt("nClient", 1);

	if (CellConfig::Instance().hasKey("-p")) {
		CellLog_Info("hasKey -p");
	}

	if (strcmp(strIP, "any") == 0) {
		strIP = nullptr;
	}

	MyServer server;
	server.InitSocket();
	server.Bind(strIP, nPort);
	server.Listen(64);
	server.Start(nThread);

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
