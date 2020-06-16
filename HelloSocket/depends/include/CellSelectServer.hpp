#ifndef _CELL_SELECT_SERVER_HPP_
#define _CELL_SELECT_SERVER_HPP_

#include "CellServer.hpp"


//网络消息接收服务类
class CellSelectServer:public CellServer {
public:
	virtual bool DoNetEvent() {
		//fd_set fdExc;
		if (_clients_change) {
			_clients_change = false;

			_fdRead.zero();
			//FD_ZERO(&fdExc);

			_maxSock = (_clients.begin()->second)->sockfd();
			for (auto iter : _clients) {
				_fdRead.add(iter.first);
				if (iter.first > _maxSock) {
					_maxSock = iter.first;
				}
			}
			_fdRead_bak.copy(_fdRead);
		}
		else {
			_fdRead.copy(_fdRead_bak);
		}

		bool bNeedWrite = false;
		_fdWrite.zero();
		for (auto iter : _clients) {
			//需要写数据的客户端，才加入fd_set检测是否可写
			if (iter.second->needWrite()) {
				_fdWrite.add(iter.first);
				bNeedWrite = true;
			}
		}

		//nfds是一个整数值，是指fd_set集合中所有描述符(socket)的范围，而不是数量
		//即是所有文件描述符最大值+1，在windows中这个参数可以写0
		timeval t = { 0, 1 }; //这是是非阻塞，将导致单核CPU达到100%
		//timeval t = { 1, 0 };
		int ret = 0;
		if (bNeedWrite) {
			ret = select(_maxSock + 1, _fdRead.fdset(), _fdWrite.fdset(), nullptr, &t);
		}
		else {
			ret = select(_maxSock + 1, _fdRead.fdset(), nullptr, nullptr, &t);
		}

		if (ret < 0) {
			if (errno == EINTR) {
				//CellLog_PError("CellSelectServer.DoSelect select error exit.\n");
				return true;
			}
			CellLog_PError("CellSelectServer.DoSelect select error exit id<%d>\n", _id);
			return false;
		}
		else if (ret == 0) {
			return true;
		}

		ReadData();
		WriteData();
		//异常有问题的是作为可写的,直接剔除
		//WriteData(fdExc);
		return true;
	}

	void ReadData() {
#ifdef _WIN32
		auto pfdset = _fdRead.fdset();
		for (int n = 0; n < pfdset->fd_count; ++n) {
			auto iter = _clients.find(pfdset->fd_array[n]);
			if (iter != _clients.end()) {
				if (SOCKET_ERROR == RecvData(iter->second)) {
					OnClientLeave(iter->second);
					_clients.erase(iter);
					continue;
				}
			}
		}
#else	
		for (auto iter = _clients.begin(); iter != _clients.end();) {
			if (_fdRead.has(iter->first)) {
				if (SOCKET_ERROR == iter->second->SendDataReal()) {
					OnClientLeave(iter->second);
					iter = _clients.erase(iter);
					continue;
				}
			}

			iter++;
		}
#endif
	}

	void WriteData() {
#ifdef _WIN32
		auto pfdset = _fdWrite.fdset();
		for (int n = 0; n < pfdset->fd_count; ++n) {
			auto iter = _clients.find(pfdset->fd_array[n]);
			if (iter != _clients.end()) {
				if (SOCKET_ERROR == iter->second->SendDataReal()) {
					OnClientLeave(iter->second);
					_clients.erase(iter);
					continue;
				}
			}
		}
#else	
		for (auto iter = _clients.begin(); iter != _clients.end();) {
			if (iter->second->needWrite() && _fdWrite.has(iter->first)) {
				if (SOCKET_ERROR == iter->second->SendDataReal()) {
					OnClientLeave(iter->second);
					iter = _clients.erase(iter);
					continue;
				}
			}

			iter++;
		}
#endif
	}
private: 
	CellFDSet _fdRead;
	CellFDSet _fdWrite;
	//备份客户socket fd_set
	CellFDSet _fdRead_bak;
	//
	SOCKET _maxSock;
};

#endif //!_CELL_SELECT_SERVER_HPP_
