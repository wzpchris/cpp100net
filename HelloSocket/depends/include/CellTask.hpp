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

#include "CellThread.hpp"

using CellTask = std::function<void()>;
//执行任务服务类型
class CellTaskServer
{
public:
	//所属serverid
	int serverId = -1;
private:
	//任务数据
	std::list<CellTask> _tasks;
	//任务数据缓冲区
	std::list<CellTask> _tasksBuf;
	//改变数据缓冲区时需要加锁
	std::mutex _mutex;
	//
	CellThread _thread;
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
		_thread.Start(nullptr, [this](CellThread* pThread) {
			OnRun(pThread);
		});
	}

	void Close() {
		//CellLog::Info("CellTaskServer.Close start serverid[%d]\n", serverId);
		_thread.Close();
		//CellLog::Info("CellTaskServer.Close end serverid[%d]\n", serverId);
	}

protected:
	//工作函数
	void OnRun(CellThread* pThread) {
		while (pThread->isRun()) {
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
				CellThread::Sleep(1);
				continue;
			}

			//处理任务
			for (auto pTask : _tasks) {
				pTask();
			}
			//清空任务
			_tasks.clear();
		}

		//处理缓冲任务
		for (auto pTask : _tasksBuf) {
			pTask();
		}

		//CellLog::Info("CellTaskServer.OnRun serverid[%d]\n", serverId);
	}
};
#endif 
