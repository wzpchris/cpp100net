#ifndef _EASY_EPOLL_SERVER_HPP_
#define _EASY_EPOLL_SERVER_HPP_

#include "EasyTcpServer.hpp"
#include "CellEpollServer.hpp"
#include "CellEpoll.hpp"

class EasyEpollServer :public EasyTcpServer {
public:
	void Start(int nCellServer) {
		EasyTcpServer::Start<CellEpollServer>(nCellServer);
	}
protected:
	//处理网络消息
	virtual void OnRun(CellThread* pThread) {
		//伯克利 BSD	socket
		CellEpoll ep;
		ep.create(_nMaxClient);
		ep.ctl(EPOLL_CTL_ADD, sockfd(), EPOLLIN);
		while (pThread->isRun()) {
			time4msg();

			int ret = ep.wait(1);
			if (ret < 0) {
				CellLog_Info("EasyEpollServer.OnRun select error exit.\n");
				pThread->Exit();
				break;
			}

			auto events = ep.events();
			for (int i = 0; i < ret; ++i) {
				if (events[i].data.fd == sockfd()){
					if (events[i].events & EPOLLIN) {
						Accpet();
					}
				}
			}
		}
	}
};

#endif //!_EASY_EPOLL_SERVER_HPP_
