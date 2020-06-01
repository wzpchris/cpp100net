#ifndef _ALLOCTOR_H_
#define _ALLOCTOR_H_

void *operator new(size_t size);
void operator delete(void *pData);

void *operator new[](size_t size);
void operator delete[](void *pData);

void *mem_alloc(size_t size);
void mem_free(void *pData);

#endif // _ALLOCTOR_H_
