#ifndef _CELL_SERVER_HPP_
#define _CELL_SERVER_HPP_

#include "Cell.hpp"
#include "INetEvent.hpp"
#include "CellClient.hpp"

#include <vector>
#include <map>

//网络消息接收服务类
class CellServer {
public:
	CellServer(SOCKET sock = INVALID_SOCKET) {
		_sock = sock;
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
			for (auto iter : _clients) {
				closesocket(iter.first);
				delete iter.second;
			}
			closesocket(_sock);
#else 
			for (auto iter : _clients) {
				close(iter.first);
				delete iter.second;
			}
			close(_sock);
#endif
			_sock = INVALID_SOCKET;
			_clients.clear();
		}
	}

	//备份客户socket fd_set
	fd_set _fdRead_bak;
	//客户列表是否有变化
	bool _clients_change;
	SOCKET _maxSock;
	//处理网络消息
	void OnRun() {
		_clients_change = true;
		while (IsRun()) {
			if (!_clientsBuff.empty())
			{
				//从缓冲队列里取出客户数据
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff) {
					_clients[pClient->sockfd()] = pClient;
				}
				_clientsBuff.clear();
				_clients_change = true;
			}

			//如果没有需要处理的客户端，就跳过
			if (_clients.empty()) {
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				//旧的时间戳
				_old_time = CellTime::getNowTimeInMilliSec();
				continue;
			}

			//伯克利 BSD	socket
			fd_set fdRead;
			FD_ZERO(&fdRead);

			if (_clients_change) {
				_maxSock = (_clients.begin()->second)->sockfd();
				for (auto iter : _clients) {
					FD_SET(iter.first, &fdRead);
					if (iter.first > _maxSock) {
						_maxSock = iter.first;
					}
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
				_clients_change = false;
			}
			else {
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}

			//nfds是一个整数值，是指fd_set集合中所有描述符(socket)的范围，而不是数量
			//即是所有文件描述符最大值+1，在windows中这个参数可以写0
			timeval t = { 0, 1 }; //这是是非阻塞，将导致单核CPU达到100%
			//timeval t = { 1, 0 };
			int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, &t);
			if (ret < 0) {
				printf("select error exit\n");
				Close();
				return;
			}
			/*else if (ret == 0) {
				continue;
			}*/
			ReadData(fdRead);
			CheckTime();
		}
	}

	time_t _old_time = CellTime::getNowTimeInMilliSec();
	void CheckTime() {
		//当前时间戳
		auto nowTime = CellTime::getNowTimeInMilliSec();
		auto dt = nowTime - _old_time;
		_old_time = nowTime;
		for (auto iter = _clients.begin(); iter != _clients.end();) {
			//心跳检测
			if (iter->second->checkHeart(dt)) {
				if (_pNetEvent) {
					_pNetEvent->OnNetLeave(iter->second);
				}

				_clients_change = true;
				delete iter->second;
				iter = _clients.erase(iter);
				continue;
			}

			//定时发送检测
			iter->second->checkSend(dt);
			++iter;
		}
	}

	void ReadData(fd_set& fdRead) {
#ifdef _WIN32
		for (int n = 0; n < fdRead.fd_count; ++n) {
			auto iter = _clients.find(fdRead.fd_array[n]);
			if (iter != _clients.end()) {
				if (-1 == RecvData(iter->second)) {
					if (_pNetEvent) {
						_pNetEvent->OnNetLeave(iter->second);
					}
					_clients_change = true;
					delete iter->second;
					_clients.erase(iter);
				}
			}
			else {
				printf("error, if (iter != _clients.end())\n ");
			}
		}
#else	
		std::vector<CellClient*> temp;
		for (auto iter : _clients) {
			if (FD_ISSET(iter.first, &fdRead)) {
				if (-1 == RecvData(iter.second)) {
					if (_pNetEvent) {
						_pNetEvent->OnNetLeave(iter.second);
					}
					_clients_change = true;
					temp.push_back(iter.second);
				}
			}
		}
		for (auto pClient : temp) {
			_clients.erase(pClient->sockfd());
			delete pClient;
		}
#endif
	}

	//缓冲区处理粘包与分包问题
	//char _szRecv[RECV_BUFF_SIZE] = {};
	//接收数据 处理粘包 拆分包
	int RecvData(CellClient *pClient) {
		//接收数据
		char *szRecv = pClient->msgBuf() + pClient->getLastPos();
		int nLen = (int)recv(pClient->sockfd(), szRecv, RECV_BUFF_SIZE - pClient->getLastPos(), 0);
		_pNetEvent->OnNetRecv(pClient);
		if (nLen <= 0) {
			//printf("client socket=%d exit\n", pClient->sockfd());
			return -1;
		}
		
		//将接收到的数据拷贝到消息缓冲区
		//memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
		//消息缓冲区的数据尾部位置后移
		pClient->setLastPos(pClient->getLastPos() + nLen);

		//判断消息缓冲区的数据长度大于消息头netmsg_DataHeader长度
		//这里的while是为了处理粘包
		while (pClient->getLastPos() >= sizeof(netmsg_DataHeader)) {
			//这时就可以知道当前消息体的长度
			netmsg_DataHeader *header = (netmsg_DataHeader*)pClient->msgBuf();
			//判断消息缓冲区的数据长度大于消息长度 
			//这里是为了处理少包问题
			if (pClient->getLastPos() >= header->dataLength) {
				//消息缓冲区剩余未处理数据的长度
				int nSize = pClient->getLastPos() - header->dataLength;
				//处理网路消息
				OnNetMsg(pClient, header);
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
	virtual void OnNetMsg(CellClient *pClient, netmsg_DataHeader *header) {
		_pNetEvent->OnNetMsg(this, pClient, header);
	}

	void addClient(CellClient *pClient) {
		//_mutex.lock();
		std::lock_guard<std::mutex> lock(_mutex);
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	void Start() {
		_thread = std::thread(std::mem_fn(&CellServer::OnRun), this);
		_taskServer.Start();
	}

	size_t getClientCount() {
		return _clients.size() + _clientsBuff.size();
	}

	/*void addSendTask(CellClient *pClient, netmsg_DataHeader *header) {
		_taskServer.addTask([pClient, header]() {
			pClient->SendData(header);
			delete header;
		});
	}*/
private:
	SOCKET _sock;
	//正式客户队列
	std::map<SOCKET, CellClient*> _clients;
	//客户缓冲区
	std::vector<CellClient*> _clientsBuff;
	//缓冲队列的锁
	std::mutex _mutex;
	std::thread _thread;
	//网络事件对象
	INetEvent *_pNetEvent;
	//
	CellTaskServer _taskServer;
};

#endif //