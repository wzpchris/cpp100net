#ifndef _CELL_THREAD_HPP_
#define _CELL_THREAD_HPP_

#include <functional>
#include <thread>
#include <mutex>
#include "CellSemaphore.hpp"

class CellThread {
public:
	static void Sleep(time_t dt) {
		std::chrono::milliseconds t(dt);
		std::this_thread::sleep_for(t);
	}
private: 
	using EventCall = std::function<void(CellThread *)>;
public:
	//启动线程
	void Start(EventCall onCreate = nullptr, EventCall onRun = nullptr, EventCall onDestory = nullptr) {
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_isRun) {
			_isRun = true;

			if (nullptr != onCreate) {
				_onCreate = onCreate;
			}

			if (nullptr != onRun) {
				_onRun = onRun;
			}

			if (nullptr != onDestory) {
				_onDestory = onDestory;
			}

			std::thread t(std::mem_fn(&CellThread::OnWork), this);
			t.detach();
		}
	}
	//关闭线程
	void Close() {
		std::lock_guard<std::mutex> lock(_mutex);
		if (_isRun) {
			_isRun = false;
			_sem.wait();
		}
	}

	//在工作函数中退出
	//不需要使用信号量来阻塞邓艾
	//如果使用会阻塞
	void Exit() {
		std::lock_guard<std::mutex> lock(_mutex);
		if (_isRun) {
			_isRun = false;
		}
	}

	bool isRun() {
		return _isRun;
	}
protected:
	//线程的运行的工作函数
	void OnWork() {
		if (nullptr != _onCreate) {
			_onCreate(this);
		}

		if (nullptr != _onRun) {
			_onRun(this);
		}

		if (nullptr != _onDestory) {
			_onDestory(this);
		}

		_sem.wakeup();
		_isRun = false;
	}
private:
	EventCall _onCreate;
	EventCall _onRun;
	EventCall _onDestory;
	//不同线程中改变数据时需要加锁
	std::mutex _mutex;
	//控制线程终止,退出
	CellSemaphore _sem;
	//线程是否启动运行中
	bool _isRun = false;
};
#endif // !_CELL_THREAD_HPP_
