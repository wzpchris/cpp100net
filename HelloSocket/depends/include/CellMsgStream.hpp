#ifndef _CELL_MSG_STREAM_HPP_
#define _CELL_MSG_STREAM_HPP_

#include "MessageHeader.hpp"
#include "CellStream.hpp"

//消息数据字节流
class CellReadStream:public CellStream {
public:
	CellReadStream(netmsg_DataHeader* header)
		:CellReadStream((char*)header, header->dataLength)
	{
	}

	CellReadStream(char* pData, int nSize, bool bDelete = false)
		: CellStream(pData, nSize, bDelete) 
	{
		push(nSize);
		//预先读取消息长度
		readInt16();
		//预先读取消息命令
		getNetCmd();
	}

	uint16_t getNetCmd() {
		uint16_t cmd = CMD_ERROR;
		read<uint16_t>(cmd);
		return cmd;
	}
};

class CellWriteStream :public CellStream {
public:
	CellWriteStream(char* pData, int nSize, bool bDelete = false)
		:CellStream(pData, nSize, bDelete)
	{
		//预先占领消息长度所需空间
		write<uint16_t>(0);
	}

	CellWriteStream(int nSize = 1024) :CellStream(nSize) {
		//预先占领消息长度所需空间
		write<uint16_t>(0);
	}
	void setNetCmd(uint16_t cmd) {
		write<uint16_t>(cmd);
	}

	bool writeString(const char* str, int len) {
		return writeArray(str, len);
	}

	bool writeString(const char* str) {
		return writeArray(str, strlen(str));
	}

	bool writeString(std::string& str) {
		return writeArray(str.c_str(), str.length());
	}

	void finish() {
		int pos = getWritePos();
		setWritePos(0);
		write<uint16_t>(pos);
		setWritePos(pos);
	}
public:

};
#endif // !_CELL_MSG_STREAM_HPP_
