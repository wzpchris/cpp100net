#ifndef _EASY_SELECT_SERVER_HPP_
#define _EASY_SELECT_SERVER_HPP_

#include "EasyTcpServer.hpp"
#include "CellSelectServer.hpp"

class EasySelectServer :public EasyTcpServer {
public:
	void Start(int nCellServer) {
		EasyTcpServer::Start<CellSelectServer>(nCellServer);
	}
protected:
	//处理网络消息
	virtual void OnRun(CellThread* pThread) {
		//伯克利 BSD	socket
		CellFDSet fdRead;
		while (pThread->isRun()) {
			time4msg();
			//清理集合
			fdRead.zero();
			//将描述符加入集合
			fdRead.add(sockfd());
			//nfds是一个整数值，是指fd_set集合中所有描述符(socket)的范围，而不是数量
			//即是所有文件描述符最大值+1，在windows中这个参数可以写0
			//timeval t = { 0, 0 }; //这是是非阻塞，将导致单核CPU达到100%
			timeval t = { 0, 1 };
			int ret = select(sockfd() + 1, fdRead.fdset(), nullptr, nullptr, &t);
			if (ret < 0) {
				if (errno == EINTR) {
					//CellLog_PError("EasySelectServer.OnRun select error exit.\n");
					continue;
				}
				CellLog::Info("EasySelectServer.OnRun select error exit.\n");
				pThread->Exit();
				break;
			}

			if (fdRead.has(sockfd())) {
				//这里可以暂时不需要了
				//fdRead.del(_sock);
				Accpet();
			}
		}
	}
};

#endif //!_EASY_SELECT_SERVER_HPP_
