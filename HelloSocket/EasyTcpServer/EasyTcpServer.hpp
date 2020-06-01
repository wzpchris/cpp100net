#ifndef _EASY_TCP_SERVER_HPP_
#define _EASY_TCP_SERVER_HPP_

#include "Cell.hpp"
#include "CellClient.hpp"
#include "INetEvent.hpp"
#include "CellServer.hpp"
#include "CellNetWork.hpp"


#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <atomic> 

class EasyTcpServer:public INetEvent {
private:
	CellThread _thread;
	//消息处理对象，内部会创建线程
	std::vector<CellServer*> _cellServers;
	//每秒消息计时
	CellTimestamp _tTime;
	SOCKET _sock;
protected:
	//recv计数
	std::atomic_int _recvCount;
	//客户端计数
	std::atomic_int _clientCount;
	//消息计数
	std::atomic_int _msgCount;
public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
		_recvCount = 0;
		_clientCount = 0;
		_msgCount = 0;
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
		}
		else {
			CellLog::Info("socket success sock=%d...\n", (int)_sock);
		}
		return _sock;
	}
	//绑定端口号
	int Bind(const char *ip, unsigned int port) {
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
		}
		else {
			_sin.sin_addr.S_un.S_addr = INADDR_ANY;
		}
#else 
		if (ip != NULL) {
			_sin.sin_addr.s_addr = inet_addr(ip);
		}
		else {
			_sin.sin_addr.s_addr = INADDR_ANY;
		}
#endif
		int ret = bind(_sock, (const sockaddr *)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret) {
			CellLog::Info("bind error...\n");
		}
		else {
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
			CellLog::Info("error invalid SOCKET sock[%d]...\n", (int)_sock);
		}
		else {
			//将新客户端分配给客户数量最少的cellServer
			addClientToCellServer(new CellClient(cSock));
			//获取ID地址 inet_ntoa(clientAddr.sin_addr)
		}

		return cSock;
	}

	void addClientToCellServer(CellClient *pClient) {
		//查找客户数量最少的
		auto pMinServer = _cellServers[0];
		for (auto pCellServer : _cellServers) {
			if (pMinServer->getClientCount() > pCellServer->getClientCount()) {
				pMinServer = pCellServer;
			}
		}
		pMinServer->addClient(pClient);
	}

	void Start(int nCellServer) {

		for (int n = 0; n < nCellServer; n++) {
			auto ser = new CellServer(n + 1);
			_cellServers.push_back(ser);
			//注册网络事件接收对象
			ser->setEventObj(this);
			//启动消息处理线程
			ser->Start();
		}

		_thread.Start(
			nullptr, 
			[this](CellThread *pThread) {
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
#ifdef _WIN32
			//关闭套接字closesocket
			closesocket(_sock);
#else 
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
			CellLog::Info("EasyTcpServer.Close end\n");
		}
	}
	
	//计算并输出每秒收到的网络消息
	void time4msg() {
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0) {
			CellLog::Info("thread<%d>, time<%lf>, sock<%d>, clients<%d>, recv<%d>, msg<%d>\n", (int)_cellServers.size(), t1, _sock, (int)_clientCount, int(_recvCount / t1), int(_msgCount / t1));
			_recvCount = 0;
			_msgCount = 0;
			_tTime.update();
		}
	}
	//只会被一个线程调用
	virtual void OnNetJoin(CellClient *pClient) {
		_clientCount++;
	}
	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetLeave(CellClient *pClient) {
		_clientCount--;
	}

	//cellServer 4 多个线程触发 不安全
	//如果只开启1个cellServer就是安全的
	virtual void OnNetMsg(CellServer *pCellServer, CellClient *pClient, netmsg_DataHeader *header) {
		_msgCount++;
	}

	virtual void OnNetRecv(CellClient *pClient) {
		_recvCount++;
	}

private:
	//处理网络消息
	void OnRun(CellThread *pThread) {
		while (pThread->isRun()) {
			time4msg();
			//伯克利 BSD	socket
			fd_set fdRead;
			FD_ZERO(&fdRead);
			FD_SET(_sock, &fdRead);
			//nfds是一个整数值，是指fd_set集合中所有描述符(socket)的范围，而不是数量
			//即是所有文件描述符最大值+1，在windows中这个参数可以写0
			//timeval t = { 0, 0 }; //这是是非阻塞，将导致单核CPU达到100%
			timeval t = { 0, 1 };
			int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);
			if (ret < 0) {
				CellLog::Info("EasyTcpServer.OnRun select error exit.\n");
				pThread->Exit();
				break;
			}

			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);
				Accpet();
			}
		}
	}
};

#endif