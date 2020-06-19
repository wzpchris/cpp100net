#ifndef _CELL_IOCP_SERVER_HPP_
#define _CELL_IOCP_SERVER_HPP_

#include "CellServer.hpp"
#include "CellIOCP.hpp"

//网络消息接收服务类
class CellIOCPServer:public CellServer {
public:
	CellIOCPServer() {
		_iocp.create();
	}
	~CellIOCPServer(){
		_iocp.destroy();	
	}

	virtual bool DoNetEvent() {
		CellClient* pClient = nullptr;
		for (auto iter = _clients.begin(); iter != _clients.end();) {
			pClient = iter->second;
			//需要写数据的客户端,才postSend
			if (pClient->needWrite()) {
				auto pIoData = pClient->makeSendIoData();
				if (pIoData) {
					if (!_iocp.postSend(pIoData)) {
						OnClientLeave(pClient);
						iter = _clients.erase(iter);
						continue;
					}
				}

				pIoData = pClient->makeRecvIoData();
				if (pIoData) {
					if (!_iocp.postRecv(pIoData)) {
						OnClientLeave(pClient);
						iter = _clients.erase(iter);
						continue;
					}
				}
			}
			else {
				auto pIoData = pClient->makeRecvIoData();
				if (pIoData) {
					if (!_iocp.postRecv(pIoData)) {
						OnClientLeave(pClient);
						iter = _clients.erase(iter);
						continue;
					}
				}
			}

			++iter;
		}
		
		while (true) {
			int ret = DoIOCPNetEvent();
			if (ret < 0) {
				return false;
			}
			else if (ret == 0) {
				break;
			}
		}

		return true;
	}

	//每次只处理一件网络事件
	//ret = -1 iocp出错
	//ret = 0 没有事件
	//ret = 1 有事件发生
	int DoIOCPNetEvent() {
		int ret = _iocp.wait(_ioEvent, 1);
		if (ret < 0) {
			CellLog_PError("CellIOCPServer.DoNetEvent error exit\n");
			return ret;
		}
		else if (ret == 0) {
			return ret;
		}

		//接收数据 
		if (IO_TYPE::RECV == _ioEvent.pIoData->iotype) {
			if (_ioEvent.bytesTrans <= 0) {
				CellLog_Error("recv rmClient close sockfd=%d, bytesTrans=%d\n", _ioEvent.pIoData->sockfd, _ioEvent.bytesTrans);
				rmClient(_ioEvent);
				return ret;
			}
			
			//
			CellClient* pClient = (CellClient*)_ioEvent.data.ptr;
			if (pClient) {
				pClient->recv4Iocp(_ioEvent.bytesTrans);
				OnNetRecv(pClient);
			}

			//CellLog_Info("recv data: sockfd=%d, bytesTrans=%d\n", _ioEvent.pIoData->sockfd, _ioEvent.bytesTrans);
		}
		//9.1 发送数据 完成
		else if (IO_TYPE::SEND == _ioEvent.pIoData->iotype) {
			//客户端断开处理
			if (_ioEvent.bytesTrans <= 0) {
				CellLog_Error("send rmClient sockfd=%d, bytesTrans=%d\n", _ioEvent.pIoData->sockfd, _ioEvent.bytesTrans);
				rmClient(_ioEvent);
				return ret;
			}

			CellClient* pClient = (CellClient*)_ioEvent.data.ptr;
			if (pClient) {
				pClient->send2Iocp(_ioEvent.bytesTrans);
			}
			
			//CellLog_Info("send data: sockfd=%d, bytesTrans=%d\n", _ioEvent.pIoData->sockfd, _ioEvent.bytesTrans);
		}
		else {
			CellLog_Waring("unknow support sockfd=%d\n", _ioEvent.data.sockfd);
		}

		return ret;
	}

	void rmClient(CellClient* pClient) {
		auto iter = _clients.find(pClient->sockfd());
		if (iter != _clients.end()) {
			_clients.erase(iter);
		}

		OnClientLeave(pClient);
	}

	void rmClient(IOCP_EVENT& ioEvent) {
		CellClient* pClient = (CellClient*)_ioEvent.data.ptr;
		if (pClient) {
			rmClient(pClient);
		}
	}

	virtual void OnClientJoin(CellClient* pClient) {
		_iocp.reg(pClient->sockfd(), pClient);
		auto pIoData = pClient->makeRecvIoData();
		if (pIoData) {
			_iocp.postRecv(pIoData);
		}
	}
private: 
	CellIOCP _iocp;
	IOCP_EVENT _ioEvent;
};

#endif //!_CELL_IOCP_SERVER_HPP_
