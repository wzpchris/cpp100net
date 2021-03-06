#ifndef _CELL_FD_SET_HPP_
#define _CELL_FD_SET_HPP_

#include "Cell.hpp"

//#define CELL_MAX_FD 10240

class CellFDSet {
public:
	CellFDSet() {

	}
	~CellFDSet() {
		destroy();
	}

	//Linux下表示socket fd的最大值
	//Windows下表示socket fd的数量
	void create(int MaxFds) {
		int nSocketNum = MaxFds;
#ifdef _WIN32 
		if (nSocketNum < 64) {
			nSocketNum = 64;
		}
 		_nfdSize = sizeof(u_int) + (sizeof(SOCKET) * nSocketNum);
#else
		// 用8KB存储65535个sockfd 8192byte * 8 = 65536
		if (nSocketNum < 65535) {
			nSocketNum = 65535;
		}
		_nfdSize = (nSocketNum / (8 * sizeof(char)) + 1);
#endif
		_pfdset = (fd_set*)new char[_nfdSize];
		memset(_pfdset, 0, _nfdSize);
		_MAX_SOCK_FD = nSocketNum;
	}

	void destroy() {
		if (_pfdset) {
			delete[] _pfdset;
			_pfdset = nullptr;
		}
	}

	void add(SOCKET s) {
#ifdef _WIN32
		if (_pfdset) {
			FD_SET(s, _pfdset);
		}
#else 
		if (s < CELL_MAX_FD) {
			if (_pfdset) {
				FD_SET(s, _pfdset);
			}
		}
		else {
			CellLog_Error("CellFDSet::add sock<%d>, CELL_MAX_FD<%d>\n", s, _MAX_SOCK_FD);
		}
#endif // _WIN32

		
	}
	void del(SOCKET s) {
		if (_pfdset) {
			FD_CLR(s, _pfdset);
		}
	}

	void zero() {
		if (_pfdset) {
#ifdef _WIN32
			FD_ZERO(_pfdset);
#else 
			memset(_pfdset, 0, _nfdSize);
#endif	
		}
	}

	bool has(SOCKET s) {
		return FD_ISSET(s, _pfdset);
	}

	fd_set* fdset() {
		return _pfdset;
	}

	size_t fdSize() {
		return _nfdSize;
	}

	void copy(CellFDSet& set) {
		_nfdSize = set.fdSize();
		memcpy(_pfdset, set.fdset(), _nfdSize);
	}
private:
	fd_set* _pfdset = nullptr;
	size_t _nfdSize = 0;
	int _MAX_SOCK_FD = 0;
};
#endif // !_CELL_FD_SET_HPP_
