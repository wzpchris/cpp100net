#ifndef _EASY_TCP_SERVER_HPP_
#define _EASY_TCP_SERVER_HPP_

#include "Cell.hpp"
#include "CellClient.hpp"
#include "INetEvent.hpp"
#include "CellServer.hpp"
#include "CellNetWork.hpp"
#include "CellConfig.hpp"

#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>

class EasyTcpServer :public INetEvent {
private:
	CellThread _thread;
	//消息处理对象，内部会创建线程
	std::vector<CellServer*> _cellServers;
	//每秒消息计时
	CellTimestamp _tTime;
	//
	SOCKET _sock;
protected:
	//客户端发送缓冲区大小
	int _nSendBuffSize;
	//客户端接收缓冲区大小
	int _nRecvBuffSize;
	//客户端连接上限
	int _nMaxClient;
	//recv计数
	std::atomic_int _recvCount;
	//客户端计数
	std::atomic_int _clientAccept;
	//消息计数
	std::atomic_int _msgCount;
	//客户端计数
	std::atomic_int _clientJoin;
public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_clientAccept = 0;
		_msgCount = 0;
		_clientJoin = 0;
		_nSendBuffSize = CellConfig::Instance().getInt("nSendBuffSize", SEND_BUFF_SIZE);
		_nRecvBuffSize = CellConfig::Instance().getInt("nRecvBuffSize", RECV_BUFF_SIZE);
		_nMaxClient = CellConfig::Instance().getInt("nMaxClient", FD_SETSIZE);
	}

	virtual ~EasyTcpServer() {
		Close();
	}

	//初始化socket
	SOCKET InitSocket() {
		CellNetWork::Init();
		//1.建立一个socket
		if (INVALID_SOCKET != _sock) {
			CellLog::Info("close before socket...\n");
			Close();
		}

		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			CellLog::Info("socket error ...\n");
		} else {
			CellNetWork::make_reuseaddr(_sock);
			CellLog::Info("socket success sock=%d...\n", (int)_sock);
		}
		return _sock;
	}
	//绑定端口号
	int Bind(const char* ip, unsigned int port) {
		if (INVALID_SOCKET == _sock) {
			InitSocket();
		}
		//2.bind绑定用户接收客户端链接的网络端口
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		if (ip != NULL) {
			_sin.sin_addr.S_un.S_addr = inet_addr(ip);
		} else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else 
		if (ip != NULL) {

			_sin.sin_addr.s_addr = inet_addr(ip);
		} else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (const sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret) {
			CellLog::Info("bind error...\n");
		} else {
			CellLog::Info("bind success port[%d]...\n", (int)port);
		}

		return ret;
	}

	//监听端口号
	int Listen(int n) {
		//3.listen监听端口
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			CellLog::Info("listen error sock[%d]...\n", (int)_sock);
		}
		else {
			CellLog::Info("listen success sock[%d]...\n", (int)_sock);
		}

		return ret;
	}

	//接收客户端连接
	SOCKET Accpet() {
		//4.accept等待接收客户端链接
		sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(clientAddr);
		SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
		cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
		cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t*)&nAddrLen);
#endif
		if (INVALID_SOCKET == cSock) {
			#ifdef _WIN32
				CellLog_Error("invalid SOCKET sock[%d]...\n", (int)cSock);
			#else
				CellLog_Error("invalid SOCKET sock[%d]...errno<%d> errMsg<%s>\n", (int)_sock, errno, strerror(errno));
			#endif
		}
		else {
			if (_clientAccept < _nMaxClient) {
				_clientAccept++;
				CellNetWork::make_reuseaddr(cSock);
				//将新客户端分配给客户数量最少的cellServer
				addClientToCellServer(new CellClient(cSock, _nSendBuffSize, _nRecvBuffSize));
				//获取ID地址 inet_ntoa(clientAddr.sin_addr)
			}
			else {
				//在这里可以获得连接的IP地址，反馈消息
				CellNetWork::destroySocket(cSock);
				CellLog_Waring("Accept to nMaxClient\n");
			}
		}

		return cSock;
	}

	void addClientToCellServer(CellClient* pClient) {
		//查找客户数量最少的
		auto pMinServer = _cellServers[0];
		for (auto pCellServer : _cellServers) {
			if (pMinServer->getClientCount() > pCellServer->getClientCount()) {
				pMinServer = pCellServer;
			}
		}
		pMinServer->addClient(pClient);
	}

	template<class ServerT>
	void Start(int nCellServer) {

		for (int n = 0; n < nCellServer; n++) {
			auto ser = new ServerT();
			ser->setId(n + 1);
			ser->setClientNum((_nMaxClient / nCellServer) + 1);
			_cellServers.push_back(ser);
			//注册网络事件接收对象
			ser->setEventObj(this);
			//启动消息处理线程
			ser->Start();
		}

		_thread.Start(
			nullptr,
			[this](CellThread* pThread) {
				OnRun(pThread);
			});
	}

	//关闭socket
	void Close() {
		if (_sock != INVALID_SOCKET) {
			CellLog::Info("EasyTcpServer.Close start\n");
			_thread.Close();
			for (auto cs : _cellServers) {
				delete cs;
			}
			_cellServers.clear();
			CellNetWork::destroySocket(_sock);
			_sock = INVALID_SOCKET;
			CellLog::Info("EasyTcpServer.Close end\n");
		}
	}

	//计算并输出每秒收到的网络消息
	void time4msg() {
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0) {
			CellLog::Info("thread<%d>, time<%lf>, sock<%d>, accpet<%d>, join<%d>, recv<%d>, msg<%d>\n", 
				(int)_cellServers.size(), 
				t1, _sock, 
				(int)_clientAccept, 
				(int)_clientJoin,
				int(_recvCount), 
				int(_msgCount));
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}
	//只会被一个线程调用
	virtual void OnNetJoin(CellClient* pClient) {
		_clientJoin++;
	}
	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetLeave(CellClient* pClient) {
		_clientAccept--;
		_clientJoin--;
	}

	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetMsg(CellServer* pCellServer, CellClient* pClient, netmsg_DataHeader* header) {
		_msgCount++;
	}

	virtual void OnNetRecv(CellClient* pClient) {
		_recvCount++;
	}

protected:
	//处理网络消息
	virtual void OnRun(CellThread* pThread) = 0;
	SOCKET sockfd() {
		return _sock;
	}
};

#endif
