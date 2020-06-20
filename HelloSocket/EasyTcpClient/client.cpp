#include "CellLog.hpp"
//#include "EasySelectClient.hpp"
#include "EasyIOCPClient.hpp"
#include "CellTimestamp.hpp"
#include "CellConfig.hpp"

#include <cstdio>
#include <thread>    //c++标准线程库
#include <atomic>

//服务端IP地址
const char* strIP = "127.0.0.1";
//服务端端口
int nPort = 4567;
//发送线程数量
int nThread = 1;
//客户端数量
int nClient = 10000;
//客户端每次发几条消息
int nMsg = 1;
//写入消息到缓冲区的间隔时间
int nSendSleep = 1;
//工作休眠时间
int nWorkSleep = 1;
//客户端发送缓冲区大小
int nSendBuffSize = SEND_BUFF_SIZE;
//客户端接收缓冲区大小
int nRecvBuffSize = RECV_BUFF_SIZE;
//是否检测发送的请求已被服务器回应
bool bCheckSendBack = true;

class MyClient :public EasyIOCPClient {
public:
	MyClient() {
		_bCheckMsgID = CellConfig::Instance().hasKey("-checkMsgID");
	}
public:
	virtual void OnNetMsg(netmsg_DataHeader* header) {
		
		_bSend = false;
		switch (header->cmd)
		{
		case CMD_LOGIN_RESULT:
		{
			netmsg_LoginR* loginret = (netmsg_LoginR*)header;
			if (_bCheckMsgID) {
				if (loginret->msgID != _nRecvMsgID) {
					//当前消息ID和本地消息次数不匹配
					CellLog_Error("OnNetMsg socket<%d> msgID<%d> _nRecvMsgID<%d> %d\n", _pClient->sockfd(), loginret->msgID, _nRecvMsgID, loginret->msgID - _nRecvMsgID);
				}
				++_nRecvMsgID;
			}
			/*CellLog_Info("recv server CMD_LOGIN_RESULT msg: [len=%d, cmd=%d, result=%d]\n",
				loginret->dataLength, loginret->cmd, loginret->result);*/
		}
		break;
		case CMD_LOGOUT_RESULT:
		{
			netmsg_LogOutR* logoutret = (netmsg_LogOutR*)header;
			/*CellLog_Info("recv server CMD_LOGOUT_RESULT msg: [len=%d, cmd=%d, result=%d]\n",
				logoutret->dataLength, logoutret->cmd, logoutret->result);*/
		}
		break;
		case CMD_NEW_USER_JOIN:
		{
			netmsg_NewUserJoin* newJoin = (netmsg_NewUserJoin*)header;
			/*CellLog_Info("recv server CMD_NEW_USER_JOIN msg: [len=%d, cmd=%d, sock=%d]\n",
				newJoin->dataLength, newJoin->cmd, newJoin->sock);*/
		}
		break;
		case CMD_ERROR:
		{
			CellLog_Info("recv error msg len=%d...\n", header->dataLength);
		}
		break;
		default: {
			CellLog_Info("recv unkown msglen=%d...\n", header->dataLength);
		}
		};
	}
	
	int SendTest(netmsg_Login* login) {
		int ret = 0;
		//如果剩余发送次数大于0
		if (_nSendCount > 0 && !_bSend) {
			login->msgID = _nSendMsgID;
			ret = SendData(login);
			if (SOCKET_ERROR != ret) {
				_bSend = bCheckSendBack;
				++_nSendMsgID;
				//如果剩余发送次数减少一次
				--_nSendCount;
			}
		}
		return ret;
	}

	bool checkSend(time_t dt) {
		_tRestTime += dt;
		//每经过nSeedSleep毫秒
		if (_tRestTime >= nSendSleep) {
			//重置计时
			_tRestTime -= nSendSleep;
			//重置发送计数
			_nSendCount = nMsg;
		}
		return _nSendCount > 0;
	}
public:
	//发送时间计数
	time_t _tRestTime = 0;
private:
	//接收消息id计数
	int _nRecvMsgID = 1;
	//发送消息id计数
	int _nSendMsgID = 1;
	//发送条数计数
	int _nSendCount = 0;
	//检查接收到的服务端消息ID是否连续
	bool _bCheckMsgID = false;
	//
	bool _bSend = false;
};

std::atomic_int sendCount(0);
std::atomic_int readyCount(0);
std::atomic_int nConnect(0);

void WorkThread(CellThread* pThread, int id) {  //4个线程 ID 1-4
	//n个线程 id为1-n
	CellLog_Info("thread<%d>, start\n", id);
	//客户端数组
	std::vector<MyClient*> clients(nClient);
	//计算本线程客户端在Clients中对应的index
	int begin = 0;
	int end = nClient;
	int nTemp1 = nSendSleep > 0 ? nSendSleep : 1;
	for (int n = begin; n < end; ++n) {
		if (!pThread->isRun()) {
			break;
		}

		clients[n] = new MyClient();
		clients[n]->_tRestTime = n % nTemp1;
		//多线程时让下CPU
		CellThread::Sleep(0);
	}

	for (int n = begin; n < end; ++n) {
		if (!pThread->isRun()) {
			break;
		}

		if (INVALID_SOCKET == clients[n]->InitSocket(nSendBuffSize, nRecvBuffSize)) {
			break;
		}

		if (SOCKET_ERROR == clients[n]->Connect(strIP, nPort)) {
			break;
		}
		nConnect++;
		CellThread::Sleep(0);
	}

	//所有连接完成
	CellLog_Info("thread<%d>, Connect<begin=%d, end=%d, nConnect=%d>\n", id, begin, end, (int)nConnect);

	readyCount++;
	while (readyCount < nThread && pThread->isRun()) { 
		//等待其他线程准备好发送数据
		CellThread::Sleep(10);
	}

	//消息
	netmsg_Login login;
	strcpy(login.UserName, "tom");
	strcpy(login.PassWord, "tom");

	//收发数据都是通过OnRun线程
	//SendData只是将数据写入发送缓冲区
	//等待select检测可写时才会发送数据
	//上次的时间点
	auto t2 = CellTime::getNowTimeInMilliSec();
	//新的时间点
	auto t0 = t2;
	//经过的时间
	auto dt = t0;
	CellTimestamp tTime;
	while (pThread->isRun()) {
		t0 = CellTime::getNowTimeInMilliSec();
		dt = t0 - t2;
		t2 = t0;
		//本次while(pThread->isRun()循环主要工作内容
		//代号work
		{
			int count = 0;
			//每轮每个客户端发送nMsg条数据
			for (int m = 0; m < nMsg; ++m) {
				//每个客户端1条1条的写入数据
				for (int n = begin; n < end; ++n) {
					if (clients[n]->IsRun()) {
						if (clients[n]->SendTest(&login) > 0) {
							++sendCount;
						}
					}
				}
			}

			//sendCount += count;
			for (int n = begin; n < end; ++n) {
				if (clients[n]->IsRun()) {
					//超时设置为0表示select检测状态后立即返回
					if (!clients[n]->OnRun(0)) {
						//连接断开
						nConnect--;
						continue;
					}
					//检测发送计数是否需要重置
					clients[n]->checkSend(dt);
				}
			}
		}
		CellThread::Sleep(nWorkSleep);
	}

	//关闭消息收发线程
	//关闭客户端
	for (int n = begin; n < end; ++n) {
		clients[n]->Close();
		delete clients[n];
	}

	CellLog_Info("thread<%d>, exit\n", id);
	--readyCount;
}

int main(int argc, char* args[]) {
	//设置运行日志名称
	CellLog::Instance().setLogPath("clientLog.log", "w", false);

	CellConfig::Instance().Init(argc, args);

	strIP = CellConfig::Instance().getStr("strIP", "127.0.0.1");
	nPort = CellConfig::Instance().getInt("nPort", 4567);
	nThread = CellConfig::Instance().getInt("nThread", 1);
	nClient = CellConfig::Instance().getInt("nClient", 10000);
	nMsg = CellConfig::Instance().getInt("nMsg", 10);
	nSendSleep = CellConfig::Instance().getInt("nSendSleep", 100);
	nWorkSleep = CellConfig::Instance().getInt("nWorkSleep", 1);
	bCheckSendBack = CellConfig::Instance().hasKey("-checkSendBack");
	nSendBuffSize = CellConfig::Instance().getInt("nSendBuffSize", SEND_BUFF_SIZE);
	nRecvBuffSize = CellConfig::Instance().getInt("nRecvBuffSize", RECV_BUFF_SIZE);
	
	//启动终端命令线程
	//用于接收运行时用户输入的指令
	CellThread tCmd;
	tCmd.Start(nullptr, [](CellThread* pThread) {
		while (pThread->isRun()) {
			char cmdBuff[256] = {};
			scanf("%s", cmdBuff);
			if (0 == strcmp(cmdBuff, "exit")) {
				pThread->Exit();
				CellLog_Info("exit cmdThread thread\n");
				break;
			}
			else {
				CellLog_Info("not support cmd\n");
			}
		}
	});

	//启动模拟客户端线程
	std::vector<CellThread*> threads;
	for (int n = 0; n < nThread; ++n) {
		CellThread* t = new CellThread();
		t->Start(nullptr, [n](CellThread* pThread) {
			WorkThread(pThread, n + 1);
		});
		threads.push_back(t);
	}

	//每秒数据统计
	CellTimestamp tTime;
	while (tCmd.isRun()) {
		auto t = tTime.getElapsedSecond();
		if (t >= 1.0) {
			CellLog_Info("thread<%d>,clients<%d>,connect<%d>,time<%lf>,send<%d>\n", nThread, nClient, (int)nConnect, t, (int)(sendCount / t));
			sendCount = 0;
			tTime.update();
		}
		CellThread::Sleep(1);
	}

	for (auto t : threads) {
		t->Close();
		delete t;
	}

	CellLog_Info("exit.\n");
	return 0;
}
