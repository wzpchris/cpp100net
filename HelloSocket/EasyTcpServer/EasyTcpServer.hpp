#ifndef _EASY_TCP_SERVER_HPP_
#define _EASY_TCP_SERVER_HPP_

#ifdef _WIN32
	#define FD_SETSIZE 2506
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <windows.h>
	#include <WinSock2.h>  //这里会产生宏重复定义问题,需要添加宏定义WIN32_LEAN_AND_MEAN
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>	
	#include <arpa/inet.h>
	#include <string.h>

	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR (-1)
#endif

#include <cstdio>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#include "MessageHeader.hpp"
#include "CellTimestamp.hpp"

//缓冲区最小单元的大小
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE 10240
#endif 

#define _CELL_THREAD_COUNT (4)

class ClientSocket {
public:
	ClientSocket(SOCKET sockfd = INVALID_SOCKET) {
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, sizeof(_szMsgBuf));
		_lastPos = 0;
	}
	SOCKET sockfd() {
		return _sockfd;
	}
	char *msgBuf() {
		return _szMsgBuf;
	}

	int getLastPos()  {
		return _lastPos;
	}
	void setLastPos(int pos) {
		_lastPos = pos;
	}
private:
	SOCKET _sockfd;  
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE * 10];
	//消息缓冲区的数据尾部位置
	int _lastPos;
};

class INetEvent {
public:
	virtual void OnLeave(ClientSocket *pClient) = 0; //纯虚函数
	virtual void OnNetMsg(SOCKET cSock, DataHeader *header) = 0;
};

class CellServer {
public:
	CellServer(SOCKET sock = INVALID_SOCKET) { 
		_sock = sock;
		_pThread = nullptr;
		_recvCount = 0;
		_pNetEvent = nullptr;
	}
	~CellServer() {
		Close();
	}

	void setEventObj(INetEvent *pNetEvent) {
		_pNetEvent = pNetEvent;
	}

	//是否工作中
	bool IsRun() {
		return _sock != INVALID_SOCKET;
	}

	//关闭socket
	void Close() {
		if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
			for (auto s : _clients) {
				closesocket(s->sockfd());
				delete s;
			}

			//8.关闭套接字closesocket
			closesocket(_sock);
			WSACleanup();
#else 
			for (auto s : _clients) {
				close(s->sockfd());
				delete s;
			}
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
			_clients.clear();
		}
	}

	//处理网络消息
	bool OnRun() {
		while (IsRun()) {
			if (_clientsBuff.size() > 0) 
			{
				//从缓冲队列里取出客户数据
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff) {
					_clients.push_back(pClient);
				}
				_clientsBuff.clear();
			}

			//如果没有需要处理的客户端，就跳过
			if (_clients.empty()) {
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			//伯克利 BSD	socket
			fd_set fdRead;
			//fd_set fdWrite;
			//fd_set fdExp;
			FD_ZERO(&fdRead);
			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExp);

			SOCKET maxSock = _clients[0]->sockfd();
			for (auto fd : _clients) {
				FD_SET(fd->sockfd(), &fdRead);
				if (fd->sockfd() > maxSock) {
					maxSock = fd->sockfd();
				}
			}

			//nfds是一个整数值，是指fd_set集合中所有描述符(socket)的范围，而不是数量
			//即是所有文件描述符最大值+1，在windows中这个参数可以写0
			//timeval t = { 0, 0 }; //这是是非阻塞，将导致单核CPU达到100%
			//timeval t = { 1, 0 };
			int ret = select(maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
			if (ret < 0) {
				printf("select error exit\n");
				Close();
				return false;
			}

			for (int n = (int)_clients.size() - 1; n >= 0; --n) {
				if (FD_ISSET(_clients[n]->sockfd(), &fdRead)) {
					if (-1 == RecvData(_clients[n])) {
						auto iter = _clients.begin() + n;
						if (iter != _clients.end()) {
							if (_pNetEvent) {
								_pNetEvent->OnLeave(_clients[n]);
							}
#ifdef _WIN32
							closesocket(_clients[n]->sockfd());
#else
							close(_clients[n]->sockfd());
#endif
							delete _clients[n];
							_clients.erase(iter);
						}
					}
				}
			}
		}
		return false;
	}

	//缓冲区处理粘包与分包问题
	char _szRecv[RECV_BUFF_SIZE] = {};
	//接收数据 处理粘包 拆分包
	int RecvData(ClientSocket *pClient) {
		//5.接收数据
		int nLen = (int)recv(pClient->sockfd(), _szRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0) {
			printf("client socket=%d exit\n", pClient->sockfd());
			return -1;
		}
		//将接收到的数据拷贝到消息缓冲区
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		//消息缓冲区的数据尾部位置后移
		pClient->setLastPos(pClient->getLastPos() + nLen);

		//判断消息缓冲区的数据长度大于消息头DataHeader长度
		//这里的while是为了处理粘包
		while (pClient->getLastPos() >= sizeof(DataHeader)) {
			//这时就可以知道当前消息体的长度
			DataHeader *header = (DataHeader*)pClient->msgBuf();
			//判断消息缓冲区的数据长度大于消息长度 
			//这里是为了处理少包问题
			if (pClient->getLastPos() >= header->dataLength) {
				//消息缓冲区剩余未处理数据的长度
				int nSize = pClient->getLastPos() - header->dataLength;
				//处理网路消息
				OnNetMsg(pClient->sockfd(), header);
				//消息缓冲区剩余未处理数据前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				pClient->setLastPos(nSize);
			}
			else {
				//剩余数据不够一条完整消息
				break;
			}
		}
		return 0;
	}
	//响应网络消息
	virtual void OnNetMsg(SOCKET cSock, DataHeader *header) {
		_recvCount++;
		_pNetEvent->OnNetMsg(cSock, header);
		//6.处理请求
		switch (header->cmd)
		{
			case CMD_LOGIN:
			{
				Login *login = (Login*)header;
				//printf("recv client msg: [len=%d, cmd=%d, username=%s, pwd=%s]\n", login->dataLength, login->cmd, login->UserName, login->PassWord);
				//忽略判断用户密码是否正确的过程
				/*LoginResult ret;
				SendData(cSock, &ret);*/
			}
			break;
			case CMD_LOGOUT:
			{
				LogOut *logout = (LogOut*)header;
				//printf("recv client msg: [len=%d, cmd=%d, username=%s]\n", logout->dataLength, logout->cmd, logout->UserName);
				//忽略判断用户密码是否正确的过程
				/*LogOutResult ret;
				SendData(cSock, &ret);*/
			}
			break;
			default:
			{
				printf("recv unknow msglen=%d...\n", header->dataLength);
				/*DataHeader ret;
				SendData(cSock, header);*/
			}
			break;
		};
	}

	void addClient(ClientSocket *pClient) {
		//_mutex.lock();
		std::lock_guard<std::mutex> lock(_mutex);
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	void Start() {
		_pThread = new std::thread(std::mem_fun(&CellServer::OnRun), this);
	}

	size_t getClientCount() {
		return _clients.size() + _clientsBuff.size();
	}
private:
	SOCKET _sock;
	//正式客户队列
	std::vector<ClientSocket*> _clients;
	//客户缓冲区
	std::vector<ClientSocket*> _clientsBuff;
	std::mutex _mutex;
	std::thread *_pThread;
	INetEvent *_pNetEvent;
public:
	std::atomic_int _recvCount;
};

class EasyTcpServer:public INetEvent {
private:
	SOCKET _sock;
	std::vector<ClientSocket*> _clients;
	std::vector<CellServer*> _cellServers;
	CellTimestamp _tTime;
public:
	EasyTcpServer() {
		_sock = INVALID_SOCKET;
	}

	virtual ~EasyTcpServer() {
		Close();
	}

	//初始化socket
	SOCKET InitSocket() {
		//启动Win Sock 2.x环境
#ifdef _WIN32
		//启动Windows socket 2.X环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		//启动，需要添加库文件ws2_32.lib
		WSAStartup(ver, &dat);
#endif
		//1.建立一个socket
		if (INVALID_SOCKET != _sock) {
			printf("close before socket...\n");
			Close();
		}

		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == _sock) {
			printf("socket error ...\n");
		}
		else {
			printf("socket success sock=%d...\n", (int)_sock);
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
			printf("bind error...\n");
		}
		else {
			printf("bind success port[%d]...\n", (int)port);
		}

		return ret;
	}

	//监听端口号
	int Listen(int n) {
		//3.listen监听端口
		int ret = listen(_sock, n);
		if (SOCKET_ERROR == ret)
		{
			printf("listen error sock[%d]...\n", (int)_sock);
		}
		else {
			printf("listen success sock[%d]...\n", (int)_sock);
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
			printf("error invalid SOCKET sock[%d]...\n", (int)_sock);
		}
		else {
			NewUserJoin userJoin;
			userJoin.sock = cSock;
			SendDataToAll(&userJoin);

			addClientToCellServer(new ClientSocket(cSock));
			//printf("new client[%d]: socket =%d, IP = %s \n", (int)_clients.size(), (int)cSock, inet_ntoa(clientAddr.sin_addr));
		}

		return cSock;
	}

	void addClientToCellServer(ClientSocket *pClient) {
		_clients.push_back(pClient);

		//查找客户数量最少的
		auto pMinServer = _cellServers[0];
		for (auto pCellServer : _cellServers) {
			if (pMinServer->getClientCount() > pCellServer->getClientCount()) {
				pMinServer = pCellServer;
			}
		}
		pMinServer->addClient(pClient);
	}

	void Start() {

		for (int n = 0; n < _CELL_THREAD_COUNT; n++) {
			auto ser = new CellServer(_sock);
			_cellServers.push_back(ser);
			ser->setEventObj(this);
			ser->Start();
		}
	}

	//关闭socket
	void Close() {
		if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
			for (auto s : _clients) {
				closesocket(s->sockfd());
				delete s;
			}

			//8.关闭套接字closesocket
			closesocket(_sock);
			WSACleanup();
#else 
			for (auto s : _clients) {
				close(s->sockfd());
				delete s;
			}
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
			_clients.clear();
		}
	}
	//处理网络消息
	bool OnRun() {
		if (IsRun()) {
			time4msg();
			//伯克利 BSD	socket
			fd_set fdRead;
			//fd_set fdWrite;
			//fd_set fdExp;
			FD_ZERO(&fdRead);
			//FD_ZERO(&fdWrite);
			//FD_ZERO(&fdExp);
			 
			FD_SET(_sock, &fdRead);
			//FD_SET(_sock, &fdWrite);
			//FD_SET(_sock, &fdExp);

			//nfds是一个整数值，是指fd_set集合中所有描述符(socket)的范围，而不是数量
			//即是所有文件描述符最大值+1，在windows中这个参数可以写0
			//timeval t = { 0, 0 }; //这是是非阻塞，将导致单核CPU达到100%
			timeval t = { 0, 10 };
			int ret = select(_sock + 1, &fdRead, nullptr, nullptr, &t);
			if (ret < 0) {
				printf("select error exit\n");
				Close();
				return false;
			}

			if (FD_ISSET(_sock, &fdRead)) {
				FD_CLR(_sock, &fdRead);
				Accpet();
				return true;
			}

			return true;
		}
		return false;
	}
	//是否工作中
	bool IsRun() {
		return _sock != INVALID_SOCKET;
	}

	//缓冲区处理粘包与分包问题
	char _szRecv[RECV_BUFF_SIZE] = {};
	//接收数据 处理粘包 拆分包
	int RecvData(ClientSocket *pClient) {
		//5.接收数据
		int nLen = (int)recv(pClient->sockfd(), _szRecv, RECV_BUFF_SIZE, 0);
		if (nLen <= 0) {
			printf("client socket=%d exit\n", pClient->sockfd());
			return -1;
		}
		//将接收到的数据拷贝到消息缓冲区
		memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		//消息缓冲区的数据尾部位置后移
		pClient->setLastPos(pClient->getLastPos() + nLen);

		//判断消息缓冲区的数据长度大于消息头DataHeader长度
		//这里的while是为了处理粘包
		while (pClient->getLastPos() >= sizeof(DataHeader)) {
			//这时就可以知道当前消息体的长度
			DataHeader *header = (DataHeader*)pClient->msgBuf();
			//判断消息缓冲区的数据长度大于消息长度 
			//这里是为了处理少包问题
			if (pClient->getLastPos() >= header->dataLength) {
				//消息缓冲区剩余未处理数据的长度
				int nSize = pClient->getLastPos() - header->dataLength;
				//处理网路消息
				//OnNetMsg(pClient->sockfd(), header);
				//消息缓冲区剩余未处理数据前移
				memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
				pClient->setLastPos(nSize);
			}
			else {
				//剩余数据不够一条完整消息
				break;
			}
		}
		return 0;
	}
	//响应网络消息
	void time4msg() {
		auto t1 = _tTime.getElapsedSecond();
		if (t1 >= 1.0) {
			int recvCount = 0;
			for (auto ser : _cellServers) {
				recvCount += ser->_recvCount;
				ser->_recvCount = 0;
			}
			printf("thread<%d>, time<%lf>, sock<%d>, clients<%d>, recvCount<%d>\n", (int)_cellServers.size(), t1, _sock, (int)_clients.size(), int(recvCount / t1));
			_tTime.update();
		}
	}
	//发送指定Socket数据
	int SendData(SOCKET cSock, DataHeader *header) {
		if (IsRun() && header) {
			return send(cSock, (const char *)header, header->dataLength, 0);
		}
		return SOCKET_ERROR;
	}
	//发送指定Socket数据
	void SendDataToAll(DataHeader *header) {
		if (IsRun() && header) {
			for (auto s : _clients) {
				SendData(s->sockfd(), header);
			}
		}
	}
	
	virtual void OnLeave(ClientSocket *pClient) {
		for (int n = (int)_clients.size() - 1; n >= 0; n--) {
			if (_clients[n] == pClient) {
				auto iter = _clients.begin() + n;
				if (iter != _clients.end()) {
					_clients.erase(iter);
				}
			}
		}
	}

	virtual void OnNetMsg(SOCKET cSock, DataHeader *header) {
		//time4msg();
	}
};

#endif