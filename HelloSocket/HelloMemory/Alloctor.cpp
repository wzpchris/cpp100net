#include "Alloctor.h"
#include "MemoryMgr.hpp"

void *operator new(size_t size) {

	return MemoryMgr::Instance().allocMem(size);
}

void operator delete(void *pData) {
	MemoryMgr::Instance().freeMem(pData);
}

void *operator new[](size_t size) {

	return MemoryMgr::Instance().allocMem(size);
}

void operator delete[](void *pData) {

	MemoryMgr::Instance().freeMem(pData);
}

void *mem_alloc(size_t size) {
	return MemoryMgr::Instance().allocMem(size);
}
void mem_free(void *pData) {
	MemoryMgr::Instance().freeMem(pData);
}