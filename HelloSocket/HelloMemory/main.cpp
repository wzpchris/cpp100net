#include "Alloctor.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <memory>
#include "CellTimestamp.hpp"
#include "CellObjectPool.hpp"

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
}

class ClassA:public ObjectPoolBase<ClassA, 10> {
public: 
	ClassA() {
		printf("ClassA\n");
	}
	~ClassA() {
		printf("~ClassA\n");
	}
	int num = 0;
};

class ClassB :public ObjectPoolBase<ClassB, 10> {
public:
	ClassB(int n) {
		num = n;
		printf("ClassA\n");
	}
	~ClassB() {
		printf("~ClassA\n");
	}
	int num = 0;
};

void fun(shared_ptr<ClassA> b) {
	
}

int main() {

	//thread t(workFun, 10);
	//t.detach(); //主线程与任务线程分离，没有关联，主线程结束后，任务线程会被迫结束
	//t.join(); //主线程要等待任务线程执行完后才继续执行

	//thread t[tCount];
	//for (int n = 0; n < tCount; n++) {
	//	t[n] = thread(workFun, n);
	//}

	//CellTimestamp tTime;
	//for (int n = 0; n < tCount; n++) {
	//	t[n].join();
	//	//t[n].detach();
	//}

	//cout << "cost time:" << tTime.getElapsedTimeInMilliSec() << endl;
	//cout << "hello, main thread." << endl;

	//int *a = new int;
	//*a = 100;
	//cout << "a=" << *a << endl;
	//delete a;

	////C++标准库智能指针的一种
	//shared_ptr<int> b = make_shared<int>();
	//*b = 100;
	//cout << "b=" << *b << endl;

	/*ClassA *a = new ClassA();
	delete a;*/

	/*shared_ptr<ClassA> b = make_shared<ClassA>();
	b->num = 100;*/

	ClassA* a1 = new ClassA();
	delete a1;

	ClassA* a2 = ClassA::createObject();
	ClassA::destroyObject(a2);

	ClassB* b1 = new ClassB(1);
	delete b1;

	ClassB* b2 = ClassB::createObject(1);
	ClassB::destroyObject(b2);

	{
		//这里使用make_shared后，在make_shared内部对所分配的字节进行了额外的包装
		//导致没有在对象池上分配，直接在内存池上分配，应该采用下面的方式
		shared_ptr<ClassA> a1 = make_shared<ClassA>();
	}

	{
		//这里会两次调用内存分配
		shared_ptr<ClassA> a1(new ClassA());
	}

	return 0;
}