#include "Alloctor.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <mutex>
#include "CellTimestamp.hpp"

using namespace std;

mutex m;
const int tCount = 4;
const int mCount = 100000;
const int nCount = mCount / tCount;;
//int sum = 0;
void workFun(int num) {
	char *data[12];
	for (size_t i = 0; i < 12; ++i) {
		data[i] = new char[rand() % 128 + 1];
	}

	for (size_t i = 0; i < 12; ++i) {
		delete[] data[i];
	}
	/*for (int n = 0; n < num; ++n) {
		cout << "hello, other thread." << endl;
	}*/
	//m.lock(); //临界区域
	//cout << "other thread " << num << endl;
	//m.unlock();
	//for (int n = 0; n < nCount; n++) {
	//	//自解锁
	//	//lock_guard<mutex> lg(m);
	//	//m.lock(); //临界区域
	//	//m.unlock();
	//}
}

int main() {

	//thread t(workFun, 10);
	//t.detach(); //主线程与任务线程分离，没有关联，主线程结束后，任务线程会被迫结束
	//t.join(); //主线程要等待任务线程执行完后才继续执行

	thread t[tCount];
	for (int n = 0; n < tCount; n++) {
		t[n] = thread(workFun, n);
	}

	CellTimestamp tTime;
	for (int n = 0; n < tCount; n++) {
		t[n].join();
		//t[n].detach();
	}

	cout << "cost time:" << tTime.getElapsedTimeInMilliSec() << endl;
	cout << "hello, main thread." << endl;

	while (true);


	return 0;
}