#ifndef _EASY_IOCP_CLIENT_HPP_
#define _EASY_IOCP_CLIENT_HPP_

#ifdef _WIN32

#ifndef CELL_USE_IOCP
#define CELL_USE_IOCP
#endif

#include "EasyTcpClient.hpp"
#include "CellIOCP.hpp"

class EasyIOCPClient:public EasyTcpClient
{
protected:
	virtual void OnInitSocket() {
		_iocp.create();
		_iocp.reg(_pClient->sockfd(), _pClient);
	}
	virtual void OnConnect() {

	}
public:
	virtual void Close() {
		_iocp.destroy();
		EasyTcpClient::Close();
	}
	//处理网络消息
	virtual bool OnRun(int microseconds /*= 1*/) {
		if (IsRun()) {
			//需要写数据的客户端,才postSend
			if (_pClient->needWrite()) {
				auto pIoData = _pClient->makeSendIoData();
				if (pIoData) {
					if (!_iocp.postSend(pIoData)) {
						Close();
						return false;
					}
				}

				pIoData = _pClient->makeRecvIoData();
				if (pIoData) {
					if (!_iocp.postRecv(pIoData)) {
						Close();
						return false;
					}
				}
			}
			else {
				auto pIoData = _pClient->makeRecvIoData();
				if (pIoData) {
					if (!_iocp.postRecv(pIoData)) {
						Close();
						return false;
					}
				}
			}

			while (true) {
				int ret = DoIOCPNetEvent(microseconds);
				if (ret < 0) {
					return false;
				}
				else if (ret == 0) {
					DoMsg();
					return true;
				}
			}
			
			return true;
		}

		return false;
	}
protected:
	int DoIOCPNetEvent(int microseconds /*= 1*/) {
		int ret = _iocp.wait(_ioEvent, microseconds);
		if (ret < 0) {
			CellLog_PError("EasyIOCPClient.DoIOCPNetEvent error exit clientId<%d> sockfd<%d>\n", _pClient->id, (int)_pClient->sockfd());
			return ret;
		}
		else if (ret == 0) {
			return ret;
		}

		//接收数据 
		if (IO_TYPE::RECV == _ioEvent.pIoData->iotype) {
			if (_ioEvent.bytesTrans <= 0) {
				CellLog_Error("recv rmClient close sockfd=%d, bytesTrans=%d\n", _ioEvent.pIoData->sockfd, _ioEvent.bytesTrans);
				Close();
				return -1;
			}

			//
			CellClient* pClient = (CellClient*)_ioEvent.data.ptr;
			if (pClient) {
				pClient->recv4Iocp(_ioEvent.bytesTrans);
			}

			//CellLog_Info("recv data: sockfd=%d, bytesTrans=%d\n", _ioEvent.pIoData->sockfd, _ioEvent.bytesTrans);
		}
		//9.1 发送数据 完成
		else if (IO_TYPE::SEND == _ioEvent.pIoData->iotype) {
			//客户端断开处理
			if (_ioEvent.bytesTrans <= 0) {
				CellLog_Error("send rmClient sockfd=%d, bytesTrans=%d\n", _ioEvent.pIoData->sockfd, _ioEvent.bytesTrans);
				Close();
				return -1;
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
protected:
	CellIOCP _iocp;
	IOCP_EVENT _ioEvent;
};

#endif //!_WIN32
#endif //!_EASY_IOCP_CLIENT_HPP_
