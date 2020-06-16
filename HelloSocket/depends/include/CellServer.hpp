#ifndef _CELL_SERVER_HPP_
#define _CELL_SERVER_HPP_

#include "Cell.hpp"
#include "INetEvent.hpp"
#include "CellClient.hpp"
#include "CellSemaphore.hpp"
#include "CellLog.hpp"
#include "CellFDSet.hpp"

#include <vector>
#include <map>

//网络消息接收服务类
class CellServer {
public:
	virtual ~CellServer() {
		CellLog::Info("CellServer exit start id[%d]\n", _id);
		Close();
		CellLog::Info("CellServer exit end id[%d]\n", _id);
	}

	void setId(int id) {
		_id = id;
		_taskServer.serverId = id;
	}

	void setEventObj(INetEvent* pNetEvent) {
		_pNetEvent = pNetEvent;
	}

	//关闭socket
	void Close() {
		CellLog::Info("CellServer[%d].Close start\n", _id);
		_taskServer.Close();
		_thread.Close();
		CellLog::Info("CellServer[%d].Close end\n", _id);

	}

	//处理网络消息
	void OnRun(CellThread* pThread) {
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
					OnClientJoin(pClient);
				}
				_clientsBuff.clear();
				_clients_change = true;
			}

			//如果没有需要处理的客户端，就跳过
			if (_clients.empty()) {
				CellThread::Sleep(1);
				//旧的时间戳
				_old_time = CellTime::getNowTimeInMilliSec();
				continue;
			}

			CheckTime();
			if (!DoNetEvent()) {
				pThread->Exit();
				break;
			}
			DoMsg();
		}

		CellLog::Info("CellServer.OnRun id[%d] exit\n", _id);
	}

	virtual bool DoNetEvent() {
		return false;
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

	virtual void OnClientJoin(CellClient* pClient) {
		
	}

	void DoMsg() {
		CellClient* pClient = nullptr;
		for (auto iter : _clients) {
			pClient = iter.second;
			//循环 判断是否有消息需要处理
			while (pClient->hasMsg()) {
				//处理网络消息
				OnNetMsg(pClient, pClient->front_msg());
				//移除 消息队列(缓冲区) 最前的一条数据
				pClient->pop_front_msg();
			}
		}
	}

	//接收数据 处理粘包 拆分包
	int RecvData(CellClient* pClient) {
		//接收数据
		int nLen = pClient->RecvData();
		if (nLen <= 0) {
			return SOCKET_ERROR;
		}
		//触发 接收到网络数据事件
		_pNetEvent->OnNetRecv(pClient);
		return 0;
	}
	//响应网络消息
	virtual void OnNetMsg(CellClient* pClient, netmsg_DataHeader* header) {
		_pNetEvent->OnNetMsg(this, pClient, header);
	}

	void addClient(CellClient* pClient) {
		//_mutex.lock();
		std::lock_guard<std::mutex> lock(_mutex);
		_clientsBuff.push_back(pClient);
		//_mutex.unlock();
	}

	void Start() {
		_taskServer.Start();
		_thread.Start(
			nullptr,
			[this](CellThread* pThread) {
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
protected: //字节大的往前，小的靠后，主要是为了字节对齐
	//正式客户队列
	std::map<SOCKET, CellClient*> _clients;
private:
	//客户缓冲区
	std::vector<CellClient*> _clientsBuff;
	//缓冲队列的锁
	std::mutex _mutex;
	//网络事件对象
	INetEvent* _pNetEvent = nullptr;
	//
	CellTaskServer _taskServer;

	//旧的时间
	time_t _old_time = CellTime::getNowTimeInMilliSec();
	//
	CellThread _thread;
	
protected:
	int _id = -1;
	//客户列表是否有变化
	bool _clients_change = true;
};

#endif //
