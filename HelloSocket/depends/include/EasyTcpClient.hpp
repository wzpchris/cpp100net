#ifndef _EASY_TCP_CLIENT_HPP_
#define _EASY_TCP_CLIENT_HPP_

#include "Cell.hpp"
#include "CellLog.hpp"
#include "CellNetWork.hpp"
#include "CellClient.hpp"
#include "CellFDSet.hpp"

class EasyTcpClient
{
public:
	EasyTcpClient() {
		_isConnect = false;
	}

	virtual ~EasyTcpClient() {
		Close();
	}

	//初始化socket
	SOCKET InitSocket(int sendSize = SEND_BUFF_SIZE, int recvSize = RECV_BUFF_SIZE) {
		CellNetWork::Init();
		//1.建立一个socket
		if (_pClient) {
			CellLog_Waring("close before socket...\n", _pClient->sockfd());
			Close();
		}

		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (INVALID_SOCKET == sock) {
			CellLog_Error("socket error ...\n");
		}
		else {
			//CellLog::Info("socket success sock=%d...\n", _sock);
			_pClient = new CellClient(sock, sendSize, recvSize);
		}
		return sock;
	}

	//连接服务器
	int Connect(const char* ip, short port) {
		if (_pClient == nullptr) {
			if (INVALID_SOCKET == InitSocket()) {
				return SOCKET_ERROR;
			}
		}
		//2.链接服务器connect
		sockaddr_in _sin = {};
		_sin.sin_family = AF_INET;
		_sin.sin_port = htons(port);
#ifdef _WIN32
		_sin.sin_addr.S_un.S_addr = inet_addr(ip);
#else 
		_sin.sin_addr.s_addr = inet_addr(ip);
#endif
		int ret = connect(_pClient->sockfd(), (const sockaddr*)&_sin, sizeof(sockaddr_in));
		if (SOCKET_ERROR == ret) {
			CellLog::Info("connect error sock=%d ip=%s:%d...\n", _pClient->sockfd(), ip, port);
		}
		else {
			_isConnect = true;
			//CellLog::Info("connect success sock=%d ip=%s:%d...\n", _sock, ip, port);
		}

		return ret;
	}

	//关闭socket
	void Close() {
		if (_pClient) {
			delete _pClient;
			_pClient = nullptr;
		}
		_isConnect = false;
	}

	//处理网络消息
	bool OnRun(int microseconds = 1) {
		if (IsRun()) {
			SOCKET _sock = _pClient->sockfd();
			
			_fdRead.zero();
			_fdWrite.zero();
			_fdRead.add(_sock);

			int ret = 0;
			timeval t = { 0, microseconds };
			if (_pClient->needWrite()) {
				_fdWrite.add(_sock);
				ret = select(_sock + 1, _fdRead.fdset(), _fdWrite.fdset(), nullptr, &t);
			}
			else {
				ret = select(_sock + 1, _fdRead.fdset() , nullptr, nullptr, &t);
			}

			if (ret < 0) {
				CellLog::Info("select error socket[%d]...\n", _sock);
				Close();
				return false;
			}

			if (_fdRead.has(_sock)) {
				if (SOCKET_ERROR == RecvData(_sock)) {
					CellLog::Info("server no msg socket[%d]\n", _sock);
					Close();
					return false;
				}
			}

			if (_fdWrite.has(_sock)) {
				if (SOCKET_ERROR == _pClient->SendDataReal()) {
					CellLog::Info("server no msg socket[%d]\n", _sock);
					Close();
					return false;
				}
			}
			return true;
		}

		return false;
	}

	//是否工作
	bool IsRun() {
		return _pClient && _isConnect;
	}

	/// <summary>
	/// //接收数据 处理粘包 拆分包
	/// </summary>
	/// <param name="cSock"></param>
	/// <returns></returns>
	int RecvData(SOCKET cSock) {
		if (IsRun()) {
			int nLen = _pClient->RecvData();
			if (nLen > 0) {
				while (_pClient->hasMsg()) {
					OnNetMsg(_pClient->front_msg());
					_pClient->pop_front_msg();
				}
			}

			return nLen;
		}
		return 0;
	}

	/// <summary>
	/// //响应网络消息
	/// </summary>
	/// <param name="header"></param>
	virtual void OnNetMsg(netmsg_DataHeader* header) = 0;

	
	//发送数据
	int SendData(netmsg_DataHeader* header) {
		if (IsRun()) {
			return _pClient->SendData(header);
		}
		return 0;
	}

	int SendData(const char* pData, int len) {
		if (IsRun()) {
			return _pClient->SendData(pData, len);
		}
		return 0;
	}
protected:
	CellClient* _pClient = nullptr;
	bool _isConnect = false;

	CellFDSet _fdRead;
	CellFDSet _fdWrite;
};

#endif 
