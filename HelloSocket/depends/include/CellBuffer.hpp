#ifndef _CELL_BUFF_HPP_
#define _CELL_BUFF_HPP_


#include "Cell.hpp"

class CellBuffer {
public:
	CellBuffer(int nSize = 8192) {
		_nSize = nSize;
		_pBuff = new char[_nSize];
	}
	~CellBuffer() {
		if (_pBuff) {
			delete[] _pBuff;
			_pBuff = nullptr;
		}
	}

	char* data() {
		return _pBuff;
	}

	bool push(const char* pData, int nLen) {
		//写入大量数据不一定要放到内存中
		//也可以存储到数据库或者磁盘中
		//if (_nLast + nLen > _nSize) {
		//	//需要写入的数据大于可用空间
		//	int n = (_nLast + nLen) - _nSize;
		//	//拓展Buff大小
		//	if (n < 8192) {
		//		n = 8192;
		//	}
		//	char* buff = new char[_nSize + n];
		//	memcpy(buff, _pBuff, _nLast);
		//	delete[] _pBuff;
		//	_pBuff = buff;
		//	_nSize += n;
		//}

		if (_nLast + nLen <= _nSize) {
			//将数据 拷贝到缓冲区尾部
			memcpy(_pBuff + _nLast, pData, nLen);
			//数据尾部变化
			_nLast += nLen;

			if (_nLast == SEND_BUFF_SIZE) {
				++_fullCount;
			}

			return true;
		}
		else {
			++_fullCount;
		}

		return false;
	}

	void pop(int nLen) {
		int n = _nLast - nLen;
		if (n > 0) {
			memcpy(_pBuff, _pBuff + nLen, n);
		}
		//这里要放在外面，放置n为0的情况
		_nLast = n;

		if (_fullCount > 0) {
			--_fullCount;
		}
	}

	int write2socket(SOCKET sockfd) {
		int ret = 0;
		//缓冲区有数据
		if (_nLast > 0 && INVALID_SOCKET != sockfd) {
			//发送数据
			ret = send(sockfd, _pBuff, _nLast, 0);
			//数据尾部位置置0
			_nLast = 0;
			_fullCount = 0;
		}
		return ret;
	}

	int read4socket(SOCKET sockfd) {
		if (_nSize - _nLast > 0) {
			//接收数据
			char *szRecv = _pBuff + _nLast;
			int nLen = (int)recv(sockfd, szRecv, _nSize - _nLast, 0);
			if (nLen <= 0) {
				return nLen;
			}
		
			//缓冲区的数据尾部位置后移
			_nLast += nLen;
			return nLen;
		}

		return 0;
	}

	bool hasMsg() {
		//判断消息缓冲区的数据长度大于消息头netmsg_DataHeader长度
		if (_nLast >= sizeof(netmsg_DataHeader)) {
			//这时就可以知道当前消息体的长度
			netmsg_DataHeader *header = (netmsg_DataHeader*)_pBuff;
			//判断消息缓冲区的数据长度大于消息长度 
			return _nLast >= header->dataLength;
		}

		return false;
	}
private:
	//缓冲区
	char* _pBuff = nullptr;
	//缓冲区的数据尾部位置，已有数据长度
	int _nLast = 0;
	//缓冲区总的空间大小，字节长度
	int _nSize = 0;
	//缓冲区写满次数计数
	int _fullCount = 0;
};

#endif // !_CELL_BUFF_HPP_