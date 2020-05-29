#ifndef _CELL_SEMAPHORE_HPP_
#define _CELL_SEMAPHORE_HPP_

#include <chrono>
#include <thread>

//简易信号量
class CellSemaphore {
public:
	void wait() {
		_isWaitExit = true;
		//阻塞等待OnRun退出
		while (_isWaitExit) {
			std::chrono::milliseconds t(1);
			std::this_thread::sleep_for(t);
		}
	}

	void wakeup() {
		if (_isWaitExit) {
			_isWaitExit = false;
		}
		else {
			printf("CellSemaphore warkup error\n");
		}
		
	}
private:
	bool _isWaitExit = false;
};
#endif // !_CELL_SEMAPHORE_HPP_
