#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

#include <mswsock.h>

#include <stdio.h>

#define nClient 2

//将AcceptEx函数加载到内存中，调用效率更高
LPFN_ACCEPTEX lpfnAcceptEx = NULL;
void loadAccpetEx(SOCKET listenSocket) {
	DWORD dwBytes = 0;
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	int iResult = WSAIoctl(listenSocket, SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx, sizeof(GuidAcceptEx),
		&lpfnAcceptEx, sizeof(lpfnAcceptEx),
		&dwBytes, NULL, NULL);
	
	if (iResult == SOCKET_ERROR) {
		printf("WSAIoctl failed with error:%u\n", WSAGetLastError());
	}
}

enum IO_TYPE {
	ACCPET = 10,
	RECV,
	SEND
};

//数据缓冲区空间大小
#define DATA_BUFF_SIZE 1024
struct IO_DATA_BASE {
	//重叠体
	OVERLAPPED overlapped;
	//
	SOCKET sockfd;
	//数据缓冲区
	char buffer[DATA_BUFF_SIZE];
	//实际缓冲区数据长度
	int length;
	//操作类型
	IO_TYPE iotype;
};

// 向IOCP投递接收客户端连接的任务
void postAccept(SOCKET sockServer, IO_DATA_BASE* pIO_Data) {
	pIO_Data->iotype = IO_TYPE::ACCPET;
	pIO_Data->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//DWORD dwBytes = 0;
	OVERLAPPED overlapped = {};
	if (FALSE == lpfnAcceptEx(sockServer,
		pIO_Data->sockfd,
		pIO_Data->buffer,
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
			printf("AccpetEx failed with error %d\n", err);
			return;
		}
	}
}

// 向IOCP投递接收数据的任务
void postRecv(IO_DATA_BASE* pIO_Data) {
	pIO_Data->iotype = IO_TYPE::RECV;
	WSABUF wsBuff = {};
	wsBuff.buf = pIO_Data->buffer;
	wsBuff.len = DATA_BUFF_SIZE;
	DWORD flags = 0;
	ZeroMemory(&pIO_Data->overlapped, sizeof(OVERLAPPED));

	if (SOCKET_ERROR == WSARecv(pIO_Data->sockfd, &wsBuff, 1, NULL, &flags,
		&pIO_Data->overlapped, NULL)) {
		int err = WSAGetLastError();
		if (ERROR_IO_PENDING != err) {
			printf("WSARecv failed with error %d\n", err);
			return;
		}
	}
}

// 向IOCP投递发送数据的任务
void postSend(IO_DATA_BASE* pIO_Data) {
	pIO_Data->iotype = IO_TYPE::SEND;
	WSABUF wsBuff = {};
	wsBuff.buf = pIO_Data->buffer;
	wsBuff.len = pIO_Data->length;
	DWORD flags = 0;
	ZeroMemory(&pIO_Data->overlapped, sizeof(OVERLAPPED));
	if (SOCKET_ERROR == WSASend(pIO_Data->sockfd, &wsBuff, 1, NULL, flags,
		&pIO_Data->overlapped, NULL)) {
		int err = WSAGetLastError();
		if (ERROR_IO_PENDING != err) {
			printf("WSASend failed with error %d\n", err);
			return;
		}
	}
}

//-- 用Socket API建立简易TCP服务器
//-- IOCP Server基础流程
int main() {
	//启动Window socket 2.x环境
	WORD ver = MAKEWORD(2, 2);
	WSADATA dat;
	WSAStartup(ver, &dat);

	//1 建立一个socket
	//当使用socket函数创建套接字时，会默认设置WSA_FLAG_OVERLAPPED标志
	///////
	//注意这里也可以用WSASocket函数创建socket
	// 最后一个参数需要设置为重叠标志(WAS_FLAG_OVERLAPPED)
	// SOCKET sockServer = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKET sockServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//2.1 设置对外IP与端口信息
	sockaddr_in _sin = {};
	_sin.sin_family = AF_INET;
	_sin.sin_port = htons(4567);
	_sin.sin_addr.s_addr = INADDR_ANY;
	//2.2 绑定sockaddr与ServerSocket
	if (SOCKET_ERROR == bind(sockServer, (sockaddr*)&_sin, sizeof(_sin))) {
		printf("error, 绑定网络端口失败...\n");
	}
	else {
		printf("绑定网络端口成功...\n");
	}

	//3 监听ServerSocket
	if (SOCKET_ERROR == listen(sockServer, 64)) {
		printf("error, 监听网络端口失败...\n");
	}
	else {
		printf("监听网络端口成功...\n");
	}

	//-------------IOCP Begin-----------------
	//4 创建完成端口IOCP
	HANDLE _completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
		NULL, 0, 0);
	if (NULL == _completionPort) {
		printf("error, 创建完成端口IOCP failed errCode(%d)\n", GetLastError());
		return -1;
	}
	//5 关联IOCP与ServerSocket
	//完成键
	auto ret = CreateIoCompletionPort((HANDLE)sockServer,
		_completionPort,
		(ULONG_PTR)sockServer, 0);
	if (!ret) {
		printf("error, 关联IOCP与ServerSocket failed errCode(%d)\n", GetLastError());
		return -1;
	}

	//6 向IOCP投递接收客户端连接的任务
	loadAccpetEx(sockServer);
	IO_DATA_BASE ioData[nClient] = {};
	for (int n = 0; n < nClient; ++n) {
		postAccept(sockServer, &ioData[n]);
	}

	int msgCount = 0;

	while (true) {
		// 获取完成端口状态
		DWORD bytesTrans = 0;
		SOCKET sock = INVALID_SOCKET;
		IO_DATA_BASE* pIOData;
		if (FALSE == GetQueuedCompletionStatus(_completionPort,
			&bytesTrans,
			(PULONG_PTR)&sock,
			(LPOVERLAPPED *)&pIOData,
			1000)) {
			int err = GetLastError();
			if (WAIT_TIMEOUT == err) {
				continue;
			}
			if (ERROR_NETNAME_DELETED == err) {
				printf("close sockfd=%d\n", pIOData->sockfd);
				closesocket(pIOData->sockfd);
				continue;
			}
			printf("GetQueuedCompletionStatus failed with error %d\n", err);
			break;
		}

		// 检查是否有事件发生，和select，epoll_wait类似
		//7.1 接收连接 完成
		if (IO_TYPE::ACCPET == pIOData->iotype) {
			printf("new client join sockfd=%d\n", pIOData->sockfd);
			//7.2 关联IOCP与ClientSocket
			auto ret = CreateIoCompletionPort((HANDLE)pIOData->sockfd,
				_completionPort,
				(ULONG_PTR)pIOData->sockfd, 0);
			if (!ret) {
				printf("close client sockfd=%d\n", pIOData->sockfd);
				closesocket(pIOData->sockfd);
				continue;
			}
			//7.3 向IOCP投递接收数据任务
			postRecv(pIOData);
		}
		//8.1 接收数据 完成 Completion
		else if (IO_TYPE::RECV == pIOData->iotype) {
			if (bytesTrans <= 0) {
				printf("recv close sockfd=%d, bytesTrans=%d\n", pIOData->sockfd, bytesTrans);
				closesocket(pIOData->sockfd);
				continue;
			}

			printf("recv client data: sockfd=%d, bytesTrans=%d, msgCount=%d\n", pIOData->sockfd, bytesTrans, ++msgCount);
			pIOData->length = bytesTrans;
			//8.2 向IOCP投递发送数据任务
			postSend(pIOData);
		}
		//9.1 发送数据 完成
		else if (IO_TYPE::SEND == pIOData->iotype) {
			//客户端断开处理
			if (bytesTrans <= 0) {
				printf("send close sockfd=%d, bytesTrans=%d\n", pIOData->sockfd, bytesTrans);
				closesocket(pIOData->sockfd);
				continue;
			}
			printf("send client data: sockfd=%d, bytesTrans=%d, msgCount=%d\n", pIOData->sockfd, bytesTrans, msgCount);
			pIOData->length = 0;
			//9.2 向IOCP投递接收数据任务
			postRecv(pIOData);
		}
		else {
			printf("未定义行为 sockfd=%d\n", sock);
		}
	}

	//-------------IOCP end---------------
	//10.1 关闭ClientSocket
	for (int n = 0; n < nClient; ++n) {
		closesocket(ioData[n].sockfd);
	}
	
	//10.2 关闭ServerSocket
	closesocket(sockServer);
	//10.3 关闭完成端口
	CloseHandle(_completionPort);

	//------------------
	//清除Windows socket环境
	WSACleanup();
	
	return 0;
}