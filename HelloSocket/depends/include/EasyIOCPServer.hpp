#ifndef _EASY_IOCP_SERVER_HPP_
#define _EASY_IOCP_SERVER_HPP_

#ifndef CELL_USE_IOCP
#define CELL_USE_IOCP
#endif

#include "EasyTcpServer.hpp"
#include "CellIOCPServer.hpp"
#include "CellIOCP.hpp"

class EasyIOCPServer :public EasyTcpServer {
public:
	void Start(int nCellServer) {
		EasyTcpServer::Start<CellIOCPServer>(nCellServer);
	}
protected:
	//处理网络消息
	virtual void OnRun(CellThread* pThread) {
		//伯克利 BSD	socket
		CellIOCP iocp;
		iocp.create();
		iocp.reg(sockfd());
		iocp.loadAccpetEx(sockfd());

		//
		//const int len = 2 * (sizeof(sockaddr_in) + 16);
		//不需要客户端再连接后立即发送数据的情况下最低长度len
		const int len = 1024;
		char buffer[len] = {};

		IO_DATA_BASE ioData = {};
		ioData.wsaBuff.buf = buffer;
		ioData.wsaBuff.len = len;
		iocp.postAccept(&ioData);

		IOCP_EVENT ioEvent = {};
		while (pThread->isRun()) {
			time4msg();

			int ret = iocp.wait(ioEvent, 1);
			if (ret < 0) {
				CellLog_Error("EasyIOCPServer.OnRun select error exit.\n");
				pThread->Exit();
				break;
			}
			if (ret == 0) {
				continue;
			}

			//7.1 接收连接 完成
			if (IO_TYPE::ACCPET == ioEvent.pIoData->iotype) {
				CellLog_Info("new client join sockfd=%d\n", ioEvent.pIoData->sockfd);
				
				IOCPAccept(ioEvent.pIoData->sockfd);
				//继续 接收连接
				iocp.postAccept(&ioData);
			}
		}
	}

	SOCKET IOCPAccept(SOCKET cSock) {
		//4.accept等待接收客户端链接
		/*sockaddr_in clientAddr = {};
		int nAddrLen = sizeof(clientAddr);*/

		if (INVALID_SOCKET == cSock) {
			CellLog_Error("invalid SOCKET sock[%d]...\n", (int)cSock);
		}
		else {
			if (_clientCount < _nMaxClient) {
				//CellNetWork::make_reuseaddr(cSock);
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
};

#endif //!_EASY_IOCP_SERVER_HPP_
