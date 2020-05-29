#ifndef _CELL_TIMESTAMP_HPP_
#define _CELL_TIMESTAMP_HPP_

#include <chrono>
using namespace std::chrono;

class CellTime {
public:
	//获取当前时间戳
	static time_t getNowTimeInMilliSec() {
		return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}
};

class CellTimestamp {
public:
	CellTimestamp() {
		update();
	}
	~CellTimestamp() {

	}

	void update() {
		_begin = high_resolution_clock::now();
	}

	// 获取当前秒
	double getElapsedSecond() {
		return getElapsedTimeInMicroSec() * 0.000001;
	}

	// 获取毫秒
	double getElapsedTimeInMilliSec() {
		return getElapsedTimeInMicroSec() * 0.001;
	}

	// 获取微秒
	long long getElapsedTimeInMicroSec() {
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}

protected:
	time_point<high_resolution_clock> _begin;
};

#endif //_CELL_TIMESTAMP_HPP_