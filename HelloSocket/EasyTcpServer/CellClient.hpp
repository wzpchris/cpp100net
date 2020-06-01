#ifndef _CELL_CLIENT_HPP_
#define _CELL_CLIENT_HPP_

#include "Cell.hpp"

//客户端心跳检测死亡计时时间
#define CLIENT_HEART_DEAD_TIME  60000
//在间隔指定时间后清空发送缓冲区
#define CLIENT_SEND_BUFF_TIME  200

// 客户端数据类型
class CellClient {
public:
	int id = -1;
	int serverId = -1;
public:
	CellClient(SOCKET sockfd = INVALID_SOCKET) {
		static int n = 1;
		id = n++;
		_sockfd = sockfd;
		memset(_szMsgBuf, 0, RECV_BUFF_SIZE);
		_lastPos = 0;
		memset(_szSendBuf, 0, SEND_BUFF_SIZE);
		_lastSendPos = 0;
		resetDtHeart();
		resetDTSend();
	}
	~CellClient() {
		printf("CellClient serverid[%d] id[%d] deconstruct\n", serverId, id);
		if (SOCKET_ERROR != _sockfd) {
#ifdef _WIN32
			closesocket(_sockfd);
#else
			close(_sockfd);
#endif
			_sockfd = SOCKET_ERROR;
		}
	}

	SOCKET sockfd() {
		return _sockfd;
	}
	char *msgBuf() {
		return _szMsgBuf;
	}

	int getLastPos() {
		return _lastPos;
	}
	void setLastPos(int pos) {
		_lastPos = pos;
	}

	//立即将发送缓冲区数据发送数据给客户端
	int SendDataReal() {
		int ret = 0;
		//缓冲区有数据
		if (_lastSendPos > 0 && INVALID_SOCKET != _sockfd) {
			//发送数据
			ret = send(_sockfd, _szSendBuf, _lastSendPos, 0);
			//数据尾部位置置0
			_lastSendPos = 0;
			_sendBuffFullCount = 0;
			resetDTSend();
		}
		return ret;
	}

	//缓冲区的控制根据业务需求的差异而调整
	//发送指定Socket数据(这里需要定时定量发送数据)
	int SendData(netmsg_DataHeader *header) {
		int ret = SOCKET_ERROR;
		//要发送的数据长度
		int nSendLen = header->dataLength;
		//要发送的数据
		const char* pSendData = (const char*)header;

		//这里是未采用select的方式，不属于异步
		//这里的while循环主要是将剩余的消息也放入发送缓冲区
		//while (true) {
		//	
		//	if (_lastSendPos + nSendLen >= SEND_BUFF_SIZE) {
		//		//计算可拷贝的数据长度
		//		int nCopyLen = SEND_BUFF_SIZE - _lastSendPos;
		//		//拷贝数据
		//		memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
		//		//计算剩余数据位置
		//		pSendData += nCopyLen;
		//		//计算剩余数据长度
		//		nSendLen -= nCopyLen;
		//		//发送数据
		//		ret = send(_sockfd, _szSendBuf, SEND_BUFF_SIZE, 0);
		//		//数据尾部位置置0
		//		_lastSendPos = 0;
		//		resetDTSend();
		//		//发送错误
		//		if (SOCKET_ERROR == ret) {
		//			return ret;
		//		}
		//	}
		//	else {
		//		//将要发送的数据 拷贝到发送缓冲区尾部
		//		memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
		//		//数据尾部变化
		//		_lastSendPos += nSendLen;
		//		break;
		//	}
		//}

		//select方式，异步
		if (_lastSendPos + nSendLen <= SEND_BUFF_SIZE) {
			//将要发送的数据 拷贝到发送缓冲区尾部
			memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
			//数据尾部变化
			_lastSendPos += nSendLen;

			if (_lastSendPos == SEND_BUFF_SIZE) {
				_sendBuffFullCount++;
			}

			return nSendLen;
		}
		else {
			_sendBuffFullCount++;
		}
		return ret;
	}

	void resetDtHeart() {
		_dtHeart = 0;
	}
	void resetDTSend() {
		_dtSend = 0;
	}
	//心跳检测
	bool checkHeart(time_t dt) {
		_dtHeart += dt;
		if (_dtHeart >= CLIENT_HEART_DEAD_TIME) {
			//printf("checkHeart deat:s=%d,time=%d\n", _sockfd, _dtHeart);
			return true;
		}
		return false;
	}

	//定时发送消息检测
	bool checkSend(time_t dt) {
		_dtSend += dt;
		if (_dtSend >= CLIENT_SEND_BUFF_TIME) {
			//printf("checkHeart deat:s=%d,time=%d\n", _sockfd, _dtSend);
			//立即将发送缓冲区的数据发送出去
			SendDataReal();
			//重置发送计时
			resetDTSend();
			return true;
		}
		return false;
	}
private:
	SOCKET _sockfd;
	//第二缓冲区 消息缓冲区
	char _szMsgBuf[RECV_BUFF_SIZE];
	//消息缓冲区的数据尾部位置
	int _lastPos;

	//发送缓冲区
	char _szSendBuf[SEND_BUFF_SIZE];
	//消息缓冲区的数据尾部位置
	int _lastSendPos;
	//心跳死亡计时
	time_t _dtHeart;
	//上次发送消息数据时间
	time_t _dtSend;
	//发送缓冲区遇到写满情况计数
	int _sendBuffFullCount = 0;
};
#endif

