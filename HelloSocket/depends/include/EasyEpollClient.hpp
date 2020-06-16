#ifndef _EASY_EPOLL_CLIENT_HPP_
#define _EASY_EPOLL_CLIENT_HPP_

#ifdef __linux__

#include "EasyTcpClient.hpp"
#include "CellEpoll.hpp"

class EasyEpollClient:public EasyTcpClient
{
public:
	virtual void Close() {
		_ep.destroy();
		EasyTcpClient::Close();
	}
	//处理网络消息
	virtual bool OnRun(int microseconds = 1) {
		if (IsRun()) {

			if (_pClient->needWrite()) {
				_ep.ctl(EPOLL_CTL_MOD, _pClient, EPOLLIN | EPOLLOUT);
			}
			else {
				_ep.ctl(EPOLL_CTL_MOD, _pClient, EPOLLIN);
			}

			int ret = _ep.wait(microseconds);
			if (ret < 0) {
				if (errno == EINTR) {
					return true;
				}
				CellLog_PError("CellEpollClient error socket[%d]...\n", _pClient->sockfd());
				Close();
				return false;
			}
			else if (ret == 0) {
				return true;
			}

			auto events = _ep.events();
			for (int i = 0; i < ret; ++i) {
				CellClient* pClient = (CellClient*)events[i].data.ptr;
				if (pClient) {
					if (events[i].events & EPOLLIN) {
						if (SOCKET_ERROR == RecvData()) {
							CellLog_Error("CellEpollClient OnRun 111 error socket[%d]...\n", _pClient->sockfd());
							Close();
							continue;
						}
					}

					if (events[i].events & EPOLLOUT) {
						if (SOCKET_ERROR == pClient->SendDataReal()) {
							CellLog_Error("CellEpollClient OnRun 222 error socket[%d]...\n", _pClient->sockfd());
							Close();
							continue;
						}
					}
				}
			}
			return true;
		}

		return false;
	}
protected:
	virtual void OnInitSocket() {
		_ep.create(1);
		_ep.ctl(EPOLL_CTL_ADD, _pClient, EPOLLIN);
	}

	virtual void OnConnect() {

	}
protected:
	CellEpoll _ep;
};

#endif //!__linux__
#endif //!_EASY_EPOLL_CLIENT_HPP_
