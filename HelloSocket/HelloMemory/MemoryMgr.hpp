#ifndef _MEMORY_MRG_HPP_
#define _MEMORY_MRG_HPP_

#include <stdlib.h>
#include <assert.h>
#include <mutex>

#ifdef _DEBUG
	#ifndef xPrintf
		#include <stdio.h>
		#define xPrintf(...) printf(__VA_ARGS__)
	#endif
#else 
	#ifndef xPrintf
		#define xPrintf(...)
	#endif
#endif //_DEBUG

#define MAX_MEMORY_SIZE 1024

class MemoryAlloc;

//内存块 最小单元
class MemoryBlock {
public:
	MemoryBlock() {
	}

	~MemoryBlock() {
	}
public:
	//内存编号
	int _nID;
	//引用次数
	int _nRef;
	//所属大内存块(池)
	MemoryAlloc *_pAlloc;
	//下一块位置
	MemoryBlock *_pNext;
	//是否在内存池中
	bool _bPool;
private:
	//预留(主要是内存对齐)
	char _unuse[3];
};

//内存池
class MemoryAlloc {
public: 
	MemoryAlloc() {
		_pBuf = nullptr;
		_pHeader = nullptr;
		_nSize = 0;
		_nBlockSize = 0;
	}
	~MemoryAlloc() {
		if (_pBuf) {
			free(_pBuf);
		}
	}

	//申请内存
	void *allocMemory(size_t size) {
		std::lock_guard<std::mutex> lock(_mutex);
		if (!_pBuf) {
			initMemory();
		}

		MemoryBlock *pReturn = nullptr;
		if (nullptr == _pHeader) {
			pReturn = (MemoryBlock*)malloc(size + sizeof(MemoryBlock));
			pReturn->_bPool = false;
			pReturn->_nID = -1;
			pReturn->_nRef = 1;
			pReturn->_pAlloc = nullptr;
			pReturn->_pNext = nullptr;
		}
		else {
			pReturn = _pHeader;
			_pHeader = _pHeader->_pNext;
			assert(0 == pReturn->_nRef);
			pReturn->_nRef = 1;
		}

		xPrintf("allocMemory: %llx, id=%d, size=%d\n", pReturn, pReturn->_nID, size);
		return ((char*)pReturn + sizeof(MemoryBlock));
	}

	//释放内存
	void freeMemory(void *pMem) {
		MemoryBlock *pBlock = (MemoryBlock*)(((char*)pMem) - sizeof(MemoryBlock));
		xPrintf("freeMemory: %llx, id=%d, ref=%d\n", pBlock, pBlock->_nID, pBlock->_nRef);
		if (pBlock->_bPool) {
			std::lock_guard<std::mutex> lock(_mutex);
			if (--(pBlock->_nRef) != 0) {
				return;
			}
			//这里pBlock与_pHeader之间可能有其他的块
			pBlock->_pNext = _pHeader;
			_pHeader = pBlock;
		}
		else {
			free(pBlock);
		}
	}

	//初始化内存池
	void initMemory() {
		xPrintf("MemoryAlloc,nSize=%d, nBlockSize=%d\n", _nSize, _nBlockSize);
		//断言
		assert(nullptr == _pBuf);
		if (_pBuf != nullptr) {
			return;
		}
		//计算内存池的大小
		size_t realSize = _nSize + sizeof(MemoryBlock);
		size_t bufSize = realSize * _nBlockSize;
		//向系统申请池的内存
		_pBuf = (char*)malloc(bufSize);

		//初始化内存池
		_pHeader = (MemoryBlock*)_pBuf;
		_pHeader->_bPool = true;
		_pHeader->_nID = 0;
		_pHeader->_nRef = 0;
		_pHeader->_pAlloc = this;
		_pHeader->_pNext = nullptr;

		//遍历内存块
		MemoryBlock *pTemp1 = _pHeader;
		for (size_t n = 1; n < _nBlockSize; ++n) {
			MemoryBlock *pTemp2 = (MemoryBlock*)(_pBuf + (n * realSize));
			pTemp2->_bPool = true;
			pTemp2->_nID = n;
			pTemp2->_nRef = 0;
			pTemp2->_pAlloc = this;
			pTemp2->_pNext = nullptr;

			pTemp1->_pNext = pTemp2;
			pTemp1 = pTemp2;
		}
	}
protected:
	//内存池地址
	char *_pBuf;
	//头部内存单元
	MemoryBlock *_pHeader;
	//内存单元的大小
	size_t _nSize;
	//内存单元的数量
	size_t _nBlockSize;
	//锁
	std::mutex _mutex;
};

//便于在声明成员变量时初始化MemoryAlloc的成员变量
template<size_t nSize, size_t nBlockSize>
class MemoryAlloctor: public MemoryAlloc {
public:
	MemoryAlloctor() {
		const size_t n = sizeof(void*);
		//这里主要是字节对齐
		//_nSize = (nSize + n - 1) / n;
		_nSize = (nSize / n) * n + (nSize % n ? n : 0);
		_nBlockSize = nBlockSize;
	}
};

//内存池管理
class MemoryMgr {
private:
	MemoryMgr() {
		init_szAlloc(0, 64, &_mem64);
		init_szAlloc(65, 128, &_mem128);
		init_szAlloc(129, 256, &_mem256);
		init_szAlloc(257, 512, &_mem512);
		init_szAlloc(513, 1024, &_mem1024);
		xPrintf("MemoryMgr.\n");
	}
	~MemoryMgr() {
	}
public:
	static MemoryMgr &Instance() {
		//单例模式,这种方式在c++11中是线程安全的
		static MemoryMgr mgr;
		return mgr;
	}
	//申请内存
	void *allocMem(size_t nSize) {
		if (nSize <= MAX_MEMORY_SIZE) {
			return _szAlloc[nSize]->allocMemory(nSize);
		}
		else {
			MemoryBlock *pReturn = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
			pReturn->_bPool = false;
			pReturn->_nID = -1;
			pReturn->_nRef = 1;
			pReturn->_pAlloc = nullptr;
			pReturn->_pNext = nullptr;
			xPrintf("allocMem: %llx, id=%d, size=%d\n", pReturn, pReturn->_nID, nSize);
			return ((char*)pReturn + sizeof(MemoryBlock));
		}
	}
	//释放内存
	void freeMem(void *pMem) {
		MemoryBlock *pBlock = (MemoryBlock*)(((char*)pMem) - sizeof(MemoryBlock));
		xPrintf("freeMem: %llx, id=%d, ref=%d\n", pBlock, pBlock->_nID, pBlock->_nRef);
		if (pBlock->_bPool) {
			pBlock->_pAlloc->freeMemory(pMem);
		}
		else {
			if (--(pBlock->_nRef) != 0) {
				return;
			}
			free(pBlock);
		}
	}

	//增加内存块的引用计数
	void addRef(void *pMem) {
		MemoryBlock *pBlock = (MemoryBlock*)(((char*)pMem) - sizeof(MemoryBlock));
		++pBlock->_nRef;
	}
private:
	//初始化内存池映射数组
	void init_szAlloc(int nBegin, int nEnd, MemoryAlloc *pMemA) {
		for (int n = nBegin; n <= nEnd; ++n) {
			_szAlloc[n] = pMemA;
		}
	}
	MemoryAlloctor<64, 10> _mem64;
	MemoryAlloctor<128, 10> _mem128;
	MemoryAlloctor<256, 10> _mem256;
	MemoryAlloctor<512, 10> _mem512;
	MemoryAlloctor<1024, 10> _mem1024;
	MemoryAlloc* _szAlloc[MAX_MEMORY_SIZE + 1];
	//MemoryAlloctor<128, 10> _mem128;
};

#endif //_MEMORY_MRG_HPP_