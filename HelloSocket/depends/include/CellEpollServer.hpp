#ifndef _CELL_EPOLL_SERVER_HPP_
#define _CELL_EPOLL_SERVER_HPP_

#include "CellServer.hpp"
#include "CellEpoll.hpp"

//网络消息接收服务类
class CellEpollServer:public CellServer {
public:
	CellEpollServer() {
		_ep.create(10240);
	}
	~CellEpollServer(){
		_ep.destroy();	
	}

	virtual bool DoNetEvent() {

		for (auto iter : _clients) {
			if (iter.second->needWrite()) {
				_ep.ctl(EPOLL_CTL_MOD, iter.second, EPOLLIN | EPOLLOUT);
			}
			else {
				_ep.ctl(EPOLL_CTL_MOD, iter.second, EPOLLIN);
			}
		}
		//
		int ret = _ep.wait(1);
		if (ret < 0) {
			CellLog_Error("CellEpollServer.DoNetEvent error exit errno<%d> errmsg<%s>\n", errno, strerror(errno));
		}else if (ret == 0) {
			return true;
		}

		//
		auto events = _ep.events();
		for (int i = 0; i < ret; ++i) {
			CellClient* pClient = (CellClient*)events[i].data.ptr;
			if (pClient) {
				if (events[i].events & EPOLLIN) {
					if (SOCKET_ERROR == RecvData(pClient)) {
						rmClient(pClient);
						continue;
					}
				}

				if (events[i].events & EPOLLOUT) {
					if (SOCKET_ERROR == pClient->SendDataReal()) {
						rmClient(pClient);
						continue;
					}
				}
			}
		}

		return true;
	}

	void rmClient(CellClient* pClient) {
		auto iter = _clients.find(pClient->sockfd());
		if (iter != _clients.end()) {
			_clients.erase(iter);
		}

		OnClientLeave(pClient);
	}

	virtual void OnClientJoin(CellClient* pClient) {
		_ep.ctl(EPOLL_CTL_ADD, pClient, EPOLLIN);
	}
private: 
	CellEpoll _ep;
};

#endif //!_CELL_EPOLL_SERVER_HPP_
