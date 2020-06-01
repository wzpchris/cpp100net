#ifndef _CELL_SERVER_HPP_
#define _CELL_SERVER_HPP_

#include "Cell.hpp"
#include "INetEvent.hpp"
#include "CellClient.hpp"
#include "CellSemaphore.hpp"

#include <vector>
#include <map>

//网络消息接收服务类
class CellServer {
public:
	CellServer(int id) {
		_id = id;
		_clients_change = true;
		_pNetEvent = nullptr;
		_taskServer.serverId = id;
	}
	~CellServer() {
		printf("CellServer exit start id[%d]\n", _id);
		Close();
		printf("CellServer exit end id[%d]\n", _id);
	}

	void setEventObj(INetEvent *pNetEvent) {
		_pNetEvent = pNetEvent;
	}

	//关闭socket
	void Close() {
		printf("CellServer[%d].Close start\n", _id);
		_taskServer.Close();
		_thread.Close();
		printf("CellServer[%d].Close end\n", _id);
		
	}

	//处理网络消息
	void OnRun(CellThread *pThread) {
		while (pThread->isRun()) {
			if (!_clientsBuff.empty())
			{
				//从缓冲队列里取出客户数据
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pClient : _clientsBuff) {
					_clients[pClient->sockfd()] = pClient;
					
					pClient->serverId = _id;
					if (_pNetEvent != nullptr) {
						_pNetEvent->OnNetJoin(pClient);
					}
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
			fd_set fdWrite;
			//fd_set fdExc;

			if (_clients_change) {
				_clients_change = false;

				FD_ZERO(&fdRead);
				FD_ZERO(&fdWrite);
				//FD_ZERO(&fdExc);

				_maxSock = (_clients.begin()->second)->sockfd();
				for (auto iter : _clients) {
					FD_SET(iter.first, &fdRead);
					if (iter.first > _maxSock) {
						_maxSock = iter.first;
					}
				}
				memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
			}
			else {
				memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
			}

			memcpy(&fdWrite, &_fdRead_bak, sizeof(fd_set));
			//memcpy(&fdExc, &_fdRead_bak, sizeof(fd_set));

			//nfds是一个整数值，是指fd_set集合中所有描述符(socket)的范围，而不是数量
			//即是所有文件描述符最大值+1，在windows中这个参数可以写0
			timeval t = { 0, 1 }; //这是是非阻塞，将导致单核CPU达到100%
			//timeval t = { 1, 0 };
			int ret = select(_maxSock + 1, &fdRead, &fdWrite, nullptr, &t);
			if (ret < 0) {
				printf("CellServer.OnRun select error exit\n");
				pThread->Exit();
				break;
			}
			/*else if (ret == 0) {
				continue;
			}*/
			ReadData(fdRead);
			WriteData(fdWrite);
			//有问题的是作为可写的,直接剔除
			//WriteData(fdExc);
			////printf("CellServer.OnRun.select id[%d]: fdRead=%d, fdWrite=%d, fdExc=%d\n", _id, fdRead.fd_count, fdWrite.fd_count, fdExc.fd_count);
			//if (fdExc.fd_count > 0) {
			//	printf("#### CellServer.OnRun.select id[%d]: fdExc=%d\n", _id, fdExc.fd_count);
			//}
			CheckTime();
		}

		printf("CellServer.OnRun id[%d] exit\n", _id);
	}

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

			////定时发送检测
			//iter->second->checkSend(dt);
			++iter;
		}
	}

	void OnClientLeave(CellClient* pClient) {
		if (_pNetEvent) {
			_pNetEvent->OnNetLeave(pClient);
		}
		_clients_change = true;
		delete pClient;
	}

	void ReadData(fd_set& fdRead) {
#ifdef _WIN32
		for (int n = 0; n < fdRead.fd_count; ++n) {
			auto iter = _clients.find(fdRead.fd_array[n]);
			if (iter != _clients.end()) {
				if (-1 == RecvData(iter->second)) {
					OnClientLeave(iter->second);
					_clients.erase(iter);
				}
			}
		}
#else	
		for (auto iter = _clients.begin(); iter != _clients.end();) {
			if (FD_ISSET(iter.first, &fdRead)) {
				if (-1 == iter->second->SendDataReal()) {
					OnClientLeave(iter->second);
					iter = _clients.erase(iter);
					continue;
				}
			}

			iter++;
		}
#endif
	}

	void WriteData(fd_set& fdWrite) {
#ifdef _WIN32
		for (int n = 0; n < fdWrite.fd_count; ++n) {
			auto iter = _clients.find(fdWrite.fd_array[n]);
			if (iter != _clients.end()) {
				if (-1 == iter->second->SendDataReal()) {
					OnClientLeave(iter->second);
					_clients.erase(iter);
				}
			}
		}
#else	
		for (auto iter = _clients.begin(); iter != _clients.end();) {
			if (FD_ISSET(iter.first, &fdRead)) {
				if (-1 == iter->second->SendDataReal()) {
					OnClientLeave(iter->second);
					iter = _clients.erase(iter);
					continue;
				}
			}

			iter++;
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
		_taskServer.Start();
		_thread.Start(
			nullptr, 
			[this](CellThread *pThread) {
				OnRun(pThread);
			},
			[this](CellThread* pThread) {
				Clearclients();
			}
		);
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
	void Clearclients() {
		for (auto iter : _clients) {
			delete iter.second;
		}

		for (auto iter : _clientsBuff) {
			delete iter;
		}
		_clientsBuff.clear();
		_clients.clear();
	}
private: //字节大的往前，小的靠后，主要是为了字节对齐
	//正式客户队列
	std::map<SOCKET, CellClient*> _clients;
	//客户缓冲区
	std::vector<CellClient*> _clientsBuff;
	//缓冲队列的锁
	std::mutex _mutex;
	//网络事件对象
	INetEvent *_pNetEvent;
	//
	CellTaskServer _taskServer;

	//备份客户socket fd_set
	fd_set _fdRead_bak;
	
	SOCKET _maxSock;

	//旧的时间
	time_t _old_time = CellTime::getNowTimeInMilliSec();
	//
	CellThread _thread;
	
	int _id = -1;
	//客户列表是否有变化
	bool _clients_change;
};

#endif //