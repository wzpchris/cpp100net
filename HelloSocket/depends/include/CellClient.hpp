#ifndef _CELL_CLIENT_HPP_
#define _CELL_CLIENT_HPP_

#include "Cell.hpp"
#include "CellBuffer.hpp"

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
	CellClient(SOCKET sockfd = INVALID_SOCKET): 
		_sendBuff(SEND_BUFF_SIZE), 
		_recvBuff(RECV_BUFF_SIZE){
		static int n = 1;
		id = n++;
		_sockfd = sockfd;

		resetDtHeart();
		resetDTSend();
	}
	~CellClient() {
		//CellLog::Info("CellClient serverid[%d] id[%d] deconstruct\n", serverId, id);
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

	int RecvData() {
		return _recvBuff.read4socket(_sockfd);
	}

	bool hasMsg() {
		return _recvBuff.hasMsg();
	}

	netmsg_DataHeader* front_msg() {
		return (netmsg_DataHeader*)_recvBuff.data();
	}

	void pop_front_msg() {
		if (hasMsg()) {
			_recvBuff.pop(front_msg()->dataLength);
		}
	}

	bool needWrite() {
		return _sendBuff.needWrite();
	}

	//立即将发送缓冲区数据发送数据给客户端
	int SendDataReal() {
		resetDTSend();
		return _sendBuff.write2socket(_sockfd);
	}

	//缓冲区的控制根据业务需求的差异而调整
	//发送指定Socket数据(这里需要定时定量发送数据)
	int SendData(netmsg_DataHeader *header) {
		if (_sendBuff.push((const char*)header, header->dataLength)) {
			return header->dataLength;
		}
		return SOCKET_ERROR;
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
			//CellLog::Info("checkHeart deat:s=%d,time=%d\n", _sockfd, _dtHeart);
			return true;
		}
		return false;
	}

	//定时发送消息检测
	bool checkSend(time_t dt) {
		_dtSend += dt;
		if (_dtSend >= CLIENT_SEND_BUFF_TIME) {
			//CellLog::Info("checkHeart deat:s=%d,time=%d\n", _sockfd, _dtSend);
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
	//消息接收缓冲区
	CellBuffer _recvBuff;
	//发送缓冲区
	CellBuffer _sendBuff;
	//心跳死亡计时
	time_t _dtHeart;
	//上次发送消息数据时间
	time_t _dtSend;
};
#endif