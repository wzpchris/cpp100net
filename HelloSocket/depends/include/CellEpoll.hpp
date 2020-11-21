#ifndef _CELL_EPOLL_HPP_
#define _CELL_EPOLL_HPP_

#if __linux__
#include "Cell.hpp"
#include "CellLog.hpp"
#include "CellClient.hpp"
#include "CellNetWork.hpp"
#include <sys/epoll.h>
#define EPOLL_ERROR (-1)

class CellEpoll {
public:
	CellEpoll() {

	}
	~CellEpoll() {
        destroy();
	}

	int create(int nMaxEvents) {
        if (_epfd > 0) {
            CellLog_Waring("Epoll_Create waring\n");
            destroy();
        }
		// linux 2.6.8后size就没有什么作用了
		// 由epoll动态管理，理论最大值为filemax
		// 通过cat /proc/sys/fs/file-max来查询
		// ulimit -n
		_epfd = epoll_create(nMaxEvents);
		if (_epfd == EPOLL_ERROR) {
			CellLog_Error("epoll_create\n");
            return _epfd;
		}

        _pEvents = new epoll_event[nMaxEvents];
        _nMaxEvents = nMaxEvents;

        return _epfd;
	}

    void destroy() {
        if (_pEvents) {
            delete[] _pEvents;
            _pEvents = nullptr;
        }

        if (_epfd > 0) {
            CellNetWork::destroySocket(_epfd);
            _epfd = -1;
        }
    }

    int ctl(int op, SOCKET sockfd, uint32_t events) {
        epoll_event ev;
        //事件类型
        ev.events = events;
        //事件关联的socket描述对象
        ev.data.fd = sockfd;
        // 向epoll对象注册需要管理，监听的socket文件描述符
        // 并且说明关心的事件
        // 返回0代表操作成功，返回负值代表失败-1
        int ret = epoll_ctl(_epfd, op, sockfd, &ev);
        if (ret == EPOLL_ERROR) {
            if (events & EPOLLIN) {
                CellLog_Error("epoll_ctl(%d, %d, %d, %d)\n", _epfd, op, sockfd, EPOLLIN);
            }

            if (events & EPOLLOUT) {
                CellLog_Error("epoll_ctl(%d, %d, %d, %d)\n", _epfd, op, sockfd, EPOLLOUT);
            }

            if (events & EPOLLERR) {
                CellLog_Error("epoll_ctl(%d, %d, %d, %d)\n", _epfd, op, sockfd, EPOLLERR);
            }

            if (events & EPOLLHUP) {
                CellLog_Error("epoll_ctl(%d, %d, %d, %d)\n", _epfd, op, sockfd, EPOLLHUP);
            }
        }

        return ret;
    }

    int ctl(int op, CellClient* pClient, uint32_t events) {
        epoll_event ev;
        //事件类型
        ev.events = events;
        //事件关联的socket描述对象
        ev.data.ptr = pClient;
        // 向epoll对象注册需要管理，监听的socket文件描述符
        // 并且说明关心的事件
        // 返回0代表操作陈宫，返回负值戴斌失败-1
        SOCKET sockfd = pClient->sockfd();
        int ret = epoll_ctl(_epfd, op, sockfd, &ev);
        if (ret == EPOLL_ERROR) {
            if (events & EPOLLIN) {
                CellLog_Error("epoll_ctl(%d, %d, %d, %d)\n", _epfd, op, sockfd, EPOLLIN);
            }

            if (events & EPOLLOUT) {
                CellLog_Error("epoll_ctl(%d, %d, %d, %d)\n", _epfd, op, sockfd, EPOLLOUT);
            }

            if (events & EPOLLERR) {
                CellLog_Error("epoll_ctl(%d, %d, %d, %d)\n", _epfd, op, sockfd, EPOLLERR);
            }

            if (events & EPOLLHUP) {
                CellLog_Error("epoll_ctl(%d, %d, %d, %d)\n", _epfd, op, sockfd, EPOLLHUP);
            }
        }

        return ret;
    }

    int wait(int timeout) {
        //epfd epoll对象的描述符
        //events 用于接收检测到的网络事件的数组
        //maxevents 接收数组的大小,能够接收的事件的数量
        //timeout 
        //  t=-1:直到有事件发生才返回
        //  t=0:立即返回
        //  t>0:等待指定数值毫秒后返回
        int ret = epoll_wait(_epfd, _pEvents, _nMaxEvents, timeout);
        if (EPOLL_ERROR == ret) {
			// 可能epoll_wait被更高级的中断给打断
            if (errno == EINTR) {
                return 0;
            }
            CellLog_Error("epoll_wait ret %d, errno<%d>, errmsg<%s>\n", ret, errno, strerror(errno));
        }
        return ret;
    }

    epoll_event* events() {
        return _pEvents;
    }

private:
    //用于接收检测到的网络事件的数据
    epoll_event* _pEvents = nullptr;
    //
    int _nMaxEvents = -1;
    int _epfd = -1;
};

#endif //!__linux__
#endif // !_CELL_EPOLL_HPP_
