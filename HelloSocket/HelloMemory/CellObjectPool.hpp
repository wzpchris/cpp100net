#ifndef _CELL_OBJECT_POOL_HPP_
#define _CELL_OBJECT_POOL_HPP_

#include <stdlib.h>
#include <mutex>
#include <assert.h>

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

template<class Type, size_t nPoolSize>
class CellObjectPool {
public:
	CellObjectPool() {
		_pBuf = nullptr;
		initPool();
	}

	~CellObjectPool() {
		if (_pBuf) {
			delete[] _pBuf;
		}	
	}
private:
	class NodeHeader {
	public:
		//下一块位置
		NodeHeader* pNext;
		//块编号
		int nID;
		//引用次数
		char nRef;
		//是否在池中
		bool bPool;
	private:
		char unuse[2];  //内存对齐
	};
public:
	//释放对象内存
	void freeObjMemory(void *pMem) {
		NodeHeader *pBlock = (NodeHeader*)(((char*)pMem) - sizeof(NodeHeader));
		xPrintf("freeObjMemory: %llx, id=%d, ref=%d\n", pBlock, pBlock->nID, pBlock->nRef);
		assert(1 == pBlock->nRef);
		if (pBlock->bPool) {
			std::lock_guard<std::mutex> lock(_mutex);
			if (--pBlock->nRef != 0) {
				return;
			}
			//这里pBlock与_pHeader之间可能有其他的块
			pBlock->pNext = _pHeader;
			_pHeader = pBlock;
		}
		else {
			if (--pBlock->nRef != 0) {
				return;
			}
			delete[] pBlock;
		}
	}
	//申请对象内存
	void *allocObjMemory(size_t size) {
		std::lock_guard<std::mutex> lock(_mutex);
		NodeHeader *pReturn = nullptr;
		if (nullptr == _pHeader) {
			pReturn = (NodeHeader*)new char[sizeof(Type) + sizeof(NodeHeader)];
			pReturn->bPool = false;
			pReturn->nID = -1;
			pReturn->nRef = 1;
			pReturn->pNext = nullptr;
		}
		else {
			pReturn = _pHeader;
			_pHeader = _pHeader->pNext;
			assert(0 == pReturn->nRef);
			pReturn->nRef = 1;
		}

		xPrintf("allocObjMemory: %llx, id=%d, size=%d\n", pReturn, pReturn->nID, size);
		return ((char*)pReturn + sizeof(NodeHeader));
	}
private:
	//初始化对象池
	void initPool() {
		assert(nullptr == _pBuf);
		if (_pBuf) {
			return;
		}
		//计算对象池的大小
		size_t nRealSize = sizeof(Type) + sizeof(NodeHeader);
		size_t nTotalSize = nPoolSize * nRealSize;
		//申请池的内齿
		_pBuf = new char[nTotalSize];
		//初始化池
		_pHeader = (NodeHeader*)_pBuf;
		_pHeader->bPool = true;
		_pHeader->nID = 0;
		_pHeader->nRef = 0;
		_pHeader->pNext = nullptr;

		//遍历进行初始化
		NodeHeader* pTemp1 = _pHeader;
		for (int n = 1; n < nPoolSize; ++n) {
			NodeHeader *pTemp2 = (NodeHeader*)(_pBuf + n * nRealSize);
			pTemp2->bPool = true;
			pTemp2->nID = n;
			pTemp2->nRef = 0;
			pTemp2->pNext = nullptr;

			pTemp1->pNext = pTemp2;
			pTemp1 = pTemp2;
		}
	}
private:
	//头部地址
	NodeHeader *_pHeader;
	//对象池内存缓存区地址
	char* _pBuf;
	//锁
	std::mutex _mutex;
};

template<class Type, size_t nPoolSize>
class ObjectPoolBase {
public:
	void* operator new(size_t nSize) {
		return objectPool().allocObjMemory(nSize);
	}

	void operator delete(void *p) {
		objectPool().freeObjMemory(p);
	}

	//不定参数 可变参数
	template<typename ...Args>
	static Type* createObject(Args ... args) {
		Type* t = new Type(args...);
		//TODO:
		return t;
	}

	static void destroyObject(Type *obj) {
		delete obj;
	}
private:
	using ClassTypePool = CellObjectPool<Type, nPoolSize>;
	static ClassTypePool& objectPool() {
		//静态CellObjectPool对象
		static ClassTypePool sPool;
		return sPool;
	}
};
#endif //_CELL_OBJECT_POOL_HPP_