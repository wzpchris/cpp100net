#ifndef _CELL_IOCP_HPP_
#define _CELL_IOCP_HPP_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

#include <mswsock.h>
#include <stdio.h>

#include "CellLog.hpp"

enum IO_TYPE {
	ACCPET = 10,
	RECV,
	SEND
};

//数据缓冲区空间大小
//#define IO_DATA_BUFF_SIZE 1024
struct IO_DATA_BASE {
	//重叠体
	OVERLAPPED overlapped;
	//
	SOCKET sockfd;
	//数据缓冲区
	WSABUF wsaBuff;
	//操作类型
	IO_TYPE iotype;
};

struct IOCP_EVENT {
	union {
		void* ptr;
		SOCKET sockfd;
	}data;

	IO_DATA_BASE* pIoData;
	DWORD bytesTrans = 0;
};

class CellIOCP {
public:
	~CellIOCP() {
		destroy();
	}

	//创建完成端口IOCP
	bool create() {
		_completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
			NULL, 0, 0);
		if (NULL == _completionPort) {
			CellLog_Error("CreateIoCompletionPort create failed errCode(%d)\n", GetLastError());
			return false;
		}

		return true;
	}

	void destroy() {
		if (_completionPort) {
			CloseHandle(_completionPort);
			_completionPort = NULL;
		}
	}

	//关联IOCP与sockfd
	bool reg(SOCKET sockfd) {
		auto ret = CreateIoCompletionPort((HANDLE)sockfd,
			_completionPort,
			(ULONG_PTR)sockfd, 0);
		if (!ret) {
			CellLog_Error("CreateIoCompletionPort reg failed errCode(%d)\n", GetLastError());
			return false;
		}

		return true;
	}

	//关联IOCP自定义数据
	bool reg(SOCKET sockfd, void* pData) {
		auto ret = CreateIoCompletionPort((HANDLE)sockfd,
			_completionPort,
			(ULONG_PTR)pData, 0);
		if (!ret) {
			CellLog_Error("CreateIoCompletionPort reg pdata failed errCode(%d)\n", GetLastError());
			return false;
		}

		return true;
	}

	// 向IOCP投递接收客户端连接的任务
	bool postAccept(IO_DATA_BASE* pIO_Data) {
		if (!_AcceptEx) {
			CellLog_Error("postAccept, _AcceptEx is NULL\n");
			return false;
		}

		pIO_Data->iotype = IO_TYPE::ACCPET;
		pIO_Data->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		OVERLAPPED overlapped = {};
		if (FALSE == _AcceptEx(_sockServer,
			pIO_Data->sockfd,
			pIO_Data->wsaBuff.buf,
			//sizeof(buffer) - (sizeof(sockaddr_in) + 16) * 2,
			0,
			sizeof(sockaddr_in) + 16,
			sizeof(sockaddr_in) + 16,
			//&dwBytes,
			NULL,
			&pIO_Data->overlapped
		)) {
			int err = WSAGetLastError();
			if (ERROR_IO_PENDING != err) {
				CellLog_Error("AccpetEx failed with error %d\n", err);
				return false;
			}
		}
		return true;
	}

	// 向IOCP投递接收数据的任务
	bool postRecv(IO_DATA_BASE* pIO_Data) {
		pIO_Data->iotype = IO_TYPE::RECV;
		DWORD flags = 0;
		ZeroMemory(&pIO_Data->overlapped, sizeof(OVERLAPPED));

		if (SOCKET_ERROR == WSARecv(pIO_Data->sockfd, &pIO_Data->wsaBuff, 1, NULL, &flags,
			&pIO_Data->overlapped, NULL)) {
			int err = WSAGetLastError();
			if (ERROR_IO_PENDING != err) {
				if (WSAECONNRESET == err) {
					return false;
				}
				CellLog_Error("WSARecv failed with error %d\n", err);
				return false;
			}
		}
		return true;
	}

	// 向IOCP投递发送数据的任务
	bool postSend(IO_DATA_BASE* pIO_Data) {
		pIO_Data->iotype = IO_TYPE::SEND;
		DWORD flags = 0;
		ZeroMemory(&pIO_Data->overlapped, sizeof(OVERLAPPED));
		if (SOCKET_ERROR == WSASend(pIO_Data->sockfd, &pIO_Data->wsaBuff, 1, NULL, flags,
			&pIO_Data->overlapped, NULL)) {
			int err = WSAGetLastError();
			if (ERROR_IO_PENDING != err) {
				if (WSAECONNRESET == err) {
					return false;
				}
				CellLog_Error("WSASend failed with error %d\n", err);
				return false;
			}
		}
		return true;
	}

	int wait(IOCP_EVENT& ioEvent, int timeout) {
		ioEvent.bytesTrans = 0;
		ioEvent.pIoData = NULL;
		ioEvent.data.ptr = NULL;
		// 获取完成端口状态
		if (FALSE == GetQueuedCompletionStatus(_completionPort,
			&ioEvent.bytesTrans,
			(PULONG_PTR)&ioEvent.data,
			(LPOVERLAPPED*)&ioEvent.pIoData,
			timeout)) {
			int err = GetLastError();
			if (WAIT_TIMEOUT == err) {
				return 0;
			}
			if (ERROR_NETNAME_DELETED == err) {
				return 1;
			}
			if (ERROR_CONNECTION_ABORTED == err) {
				return 1;
			}
			if (ERROR_SEM_TIMEOUT == err) {
				return 1;
			}
			CellLog_Error("GetQueuedCompletionStatus failed with error %d\n", err);
			return -1;
		}
		return 1;
	}

	bool loadAccpetEx(SOCKET listenSocket) {
		if (INVALID_SOCKET != _sockServer) {
			CellLog_Error("loadAccpetEx _sockServer != INVALID_SOCKET\n");
			return false;
		}
		if (_AcceptEx) {
			CellLog_Error("loadAccpetEx _AcceptEx != NULL\n");
			return false;
		}
		
		_sockServer = listenSocket;
		DWORD dwBytes = 0;
		GUID GuidAcceptEx = WSAID_ACCEPTEX;
		int iResult = WSAIoctl(listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GuidAcceptEx, sizeof(GuidAcceptEx),
			&_AcceptEx, sizeof(_AcceptEx),
			&dwBytes, NULL, NULL);

		if (iResult == SOCKET_ERROR) {
			CellLog_Error("WSAIoctl failed with error:%u\n", WSAGetLastError());
			return false;
		}

		return true;
	}
private:
	//将AcceptEx函数加载到内存中，调用效率更高
	LPFN_ACCEPTEX _AcceptEx = NULL;
	HANDLE _completionPort = NULL;
	SOCKET _sockServer = INVALID_SOCKET;
};

#endif //!_WIN32
#endif //!_CELL_IOCP_HPP_