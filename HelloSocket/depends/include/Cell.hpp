#ifndef _CELL_HPP_
#define _CELL_HPP_

//socket
#ifdef _WIN32
	#define FD_SETSIZE 2506
	#define WIN32_LEAN_AND_MEAN
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <windows.h>
	#include <WinSock2.h>  //这里会产生宏重复定义问题,需要添加宏定义WIN32_LEAN_AND_MEAN
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>	
	#include <arpa/inet.h>
	#include <string.h>
	#include <signal.h>

	#define SOCKET int
	#define INVALID_SOCKET (SOCKET)(~0)
	#define SOCKET_ERROR (-1)
#endif


//自定义
#include "CellLog.hpp"
#include "MessageHeader.hpp"
#include "CellTimestamp.hpp"
#include "CellTask.hpp"


#include <cstdio>

//缓冲区最小单元的大小
#ifndef RECV_BUFF_SIZE
#define RECV_BUFF_SIZE (10240)
#define SEND_BUFF_SIZE 102400
#endif

#endif // _CELL_HPP_