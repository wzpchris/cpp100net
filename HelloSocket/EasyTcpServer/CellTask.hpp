/*
	v1.0
*/

#ifndef _CELL_TASK_HPP_
#define _CELL_TASK_HPP_

#include <thread>
#include <mutex>
#include <list>
#include <vector>
#include <functional>

using CellTask = std::function<void()>;
//执行任务服务类型
class CellTaskServer
{
private:
	//任务数据
	std::list<CellTask> _tasks;
	//任务数据缓冲区
	std::list<CellTask> _tasksBuf;
	//改变数据缓冲区时需要加锁
	std::mutex _mutex;
	//
	bool _isRun = false;
public:
	CellTaskServer() {
	}
	~CellTaskServer() {
	}

	//添加任务
	void addTask(CellTask task) {
		std::lock_guard<std::mutex> lock(_mutex);
		_tasksBuf.push_back(task);

	}

	//启动工作线程
	void Start() {
		_isRun = true;
		std::thread t(std::mem_fn(&CellTaskServer::OnRun), this);
		t.detach();
	}

	void Close() {
		_isRun = false;
	}

protected:
	//工作函数
	void OnRun() {
		while (_isRun) {
			//从缓冲区取出数据
			if (!_tasksBuf.empty()) {
				std::lock_guard<std::mutex> lock(_mutex);
				for (auto pTask : _tasksBuf) {
					_tasks.push_back(pTask);
				}
				_tasksBuf.clear();
			}

			//如果没有任务
			if (_tasks.empty()) {
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}

			//处理任务
			for (auto pTask : _tasks) {
				pTask();
			}
			//清空任务
			_tasks.clear();
		}
	}
};
#endif 