#ifndef _CELL_NET_WORK_HPP_
#define _CELL_NET_WORK_HPP_

#include "Cell.hpp"

class CellNetWork {
private:
	CellNetWork() {
		//启动Win Sock 2.x环境
#ifdef _WIN32
		//启动Windows socket 2.X环境
		WORD ver = MAKEWORD(2, 2);
		WSADATA dat;
		//启动，需要添加库文件ws2_32.lib
		WSAStartup(ver, &dat);
#endif

#ifndef _WIN32
		/*if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
			return 1;
		}*/
		//忽略异常信号，默认情况会导致进程终止
		signal(SIGPIPE, SIG_IGN);
#endif
	}
	~CellNetWork() {
#ifdef _WIN32
		WSACleanup();
#endif
	}
public:
	static void Init() {
		static CellNetWork obj;
	}
private:

};
#endif // !_CELL_NET_WORK_HPP_
