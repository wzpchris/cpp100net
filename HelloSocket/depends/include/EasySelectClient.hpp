#ifndef _EASY_SELECT_CLIENT_HPP_
#define _EASY_SELECT_CLIENT_HPP_

#include "EasyTcpClient.hpp"
#include "CellFDSet.hpp"

class EasySelectClient:public EasyTcpClient
{
public:
	EasySelectClient() {
		_fdRead.create(1);
		_fdWrite.create(1);
	}
	//处理网络消息
	virtual bool OnRun(int microseconds = 1) {
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
				if (SOCKET_ERROR == RecvData()) {
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
protected:
	CellFDSet _fdRead;
	CellFDSet _fdWrite;
};

#endif //!_EASY_SELECT_CLIENT_HPP_
