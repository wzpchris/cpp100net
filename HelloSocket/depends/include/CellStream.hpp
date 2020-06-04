#ifndef _CELL_STREAM_HPP_
#define _CELL_STREAM_HPP_

#include "Cell.hpp"
#include <cstdint>
#include <string>

//字节流BYTE
class CellStream {
public:
	CellStream(char* pData, int nSize, bool bDelete = false) {
		_nSize = nSize;
		_pBuff = pData;
		_bDelete = bDelete;
	}
	CellStream(int nSize = 1024) {
		_nSize = nSize;
		_pBuff = new char[_nSize];
		_bDelete = true;
	}
	virtual ~CellStream() {
		if (_bDelete && _pBuff) {
			delete[] _pBuff;
			_pBuff = nullptr;
		}
	}
public:
	char* data() {
		return _pBuff;
	}
	int length() {
		return _nWritePos;
	}
	//内联函数
	//是否还能读出n字节的数据
	inline bool canRead(int n) {
		return _nWritePos - _nReadPos >= n;
	}
	//是否还能写入n字节的数据
	inline bool canWrite(int n) {
		return _nSize - _nWritePos >= n;
	}
	//已写入位置，添加n字节长度
	inline void push(int n) {
		_nWritePos += n;
	}
	//已读位置，添加n字节长度
	inline void pop(int n) {
		_nReadPos += n;
	}

	inline void setWritePos(int n) {
		_nWritePos = n;
	}

	inline int getWritePos() {
		return _nWritePos;
	}
	//// Read
	template<typename T>
	bool read(T& n, bool bOffset = true) {
		//计算要读取数据的字节长度
		auto nLen = sizeof(T);
		//判断能不能读
		if (canRead(nLen)) {
			//将要读取的数据 拷贝到缓冲区尾部
			memcpy(&n, _pBuff + _nReadPos, nLen);
			//计算已读尾部位置
			if (bOffset) {
				pop(nLen);
			}
			return true;
		}
		//断言assert
		//错误日志
		CellLog::Info("error, CellStream::read failed.\n");
		return false;
	}

	template<typename T>
	bool onlyRead(T& n) {
		return read(n, false);
	}

	template<typename T>
	uint32_t readArray(T* pArr, uint32_t len) {
		uint32_t len1 = 0;
		//读取数组元素个数，但不偏移读取位置
		read(len1, false);
		//判断缓存数组能否放得下
		if (len1 <= len) {
			//计算数组的字节长度
			auto nLen = len1 * sizeof(T);
			//判断能不能读出
			if (canRead(nLen + sizeof(uint32_t))) {
				//计算已读位置+数组长度所占用的空间
				pop(sizeof(uint32_t));
				//将要读取的数据 拷贝到缓冲区尾部
				memcpy(pArr, _pBuff + _nReadPos, nLen);
				//计算已读尾部位置
				pop(nLen);
				return len1;
			}
		}

		CellLog::Info("error, CellStream::readArray failed.\n");
		return 0;
	}

	int8_t readInt8(/*int8_t def = 0*/) {
		int8_t n = 0;
		read(n);
		return n;
	}
	int16_t readInt16() {
		int16_t n = 0;
		read(n);
		return n;
	}
	int32_t readInt32() {
		int32_t n;
		read(n);
		return n;
	}

	int64_t readInt64() {
		int64_t n;
		read(n);
		return n;
	}

	uint8_t readUInt8(/*int8_t def = 0*/) {
		uint8_t n = 0;
		read(n);
		return n;
	}
	uint16_t readUInt16() {
		uint16_t n = 0;
		read(n);
		return n;
	}
	uint32_t readUInt32() {
		uint32_t n;
		read(n);
		return n;
	}

	uint64_t readUInt64() {
		uint64_t n;
		read(n);
		return n;
	}
	float readFloat() {
		float n;
		read(n);
		return n;
	}
	double readDouble() {
		double n;
		read(n);
		return n;
	}

	////// Write
	template<typename T>
	bool write(T n) {
		//计算要写入数据的大小
		auto nLen = sizeof(T);
		//判断是否能写入
		if (canWrite(nLen)) {
			//将要写入的数据 拷贝到缓冲区尾部
			memcpy(_pBuff + _nWritePos, &n, nLen);
			//数据尾部变化
			push(nLen);
			return true;
		}

		CellLog::Info("error, CellStream::write failed.\n");
		return false;
	}

	template<typename T>
	bool writeArray(T* pData, uint32_t len) {
		//计算要写入数组的字节长度
		auto nLen = sizeof(T) * len;
		//判断是否能写入
		if (canWrite(nLen + sizeof(uint32_t))) {
			//写入数组的元素数量
			writeInt32(len);
			//将要写入的数据 拷贝到缓冲区尾部
			memcpy(_pBuff + _nWritePos, pData, nLen);
			//数据尾部变化
			push(nLen);
			return true;
		}

		CellLog::Info("error, CellStream::writeArray failed.\n");
		return false;
	}

	bool writeInt8(int8_t n) {
		return write(n);
	}
	bool writeInt16(int16_t n) {
		return write(n);
	}
	bool writeInt32(int32_t n) {
		return write(n);
	}
	bool writeFloat(float n) {
		return write(n);
	}
	bool writeDouble(double n) {
		return write(n);
	}
protected:
	//数据缓冲区
	char* _pBuff = nullptr;
	//缓冲区总的空间大小，字节长度
	int _nSize = 0;
	//已写入数据的尾部位置，已写入数据长度
	int _nWritePos = 0;
	//已读取数据的尾部位置
	int _nReadPos = 0;
	//_pBuff是外部传入的数据块时是否应该被释放
	bool _bDelete = true;
};
#endif // !_CELL_STREAM_HPP_
