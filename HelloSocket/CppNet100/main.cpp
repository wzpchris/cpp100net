#ifndef _CPP_NET_100_DLL_H_
#define _CPP_NET_100_DLL_H_

#include <string>

#include "EasyTcpClient.hpp"
#include "CellMsgStream.hpp"

#ifdef _WIN32
#define EXPORT_DLL _declspec(dllexport)
#else 
#define EXPORT_DLL
#endif // _WIN32

extern "C" {
	typedef void(*OnNetMsgCallBack)(void* csObj, void* data, int len);
}


class NativeTCPClient :public EasyTcpClient {
public:
	virtual void OnNetMsg(netmsg_DataHeader* header) {
		if (_callBack) {
			_callBack(_csObj, header, header->dataLength);
		}
	}

	void setCallBack(void* csObj, OnNetMsgCallBack cb) {
		_csObj = csObj;
		_callBack = cb;
	}
private:
	void* _csObj = nullptr;
	OnNetMsgCallBack _callBack = nullptr;
};

extern "C"
{
	EXPORT_DLL int Add(int a, int b) {
		return a + b;
	}

	typedef void(*CallBack1)(const char* str);

	EXPORT_DLL void TestCall(const char* str, CallBack1 cb) {
		std::string s = "hello ";
		s += str;
		cb(s.c_str());
	}

	///////CellClient
	EXPORT_DLL void* CellClient_Create(void* csObj, OnNetMsgCallBack cb, int sendSize, int recvSize) {
		NativeTCPClient* pClient = new NativeTCPClient();
		pClient->setCallBack(csObj, cb);
		pClient->InitSocket(sendSize, recvSize);
		return pClient;
	}

	EXPORT_DLL bool CellClient_Connect(NativeTCPClient* pClient, const char* ip, short port) {
		if (pClient && ip) {
			return SOCKET_ERROR != pClient->Connect(ip, port);
		}
		return false;
	}

	EXPORT_DLL bool CellClient_OnRun(NativeTCPClient* pClient) {
		if (pClient) {
			return pClient->OnRun();
		}
		return false;
	}

	EXPORT_DLL void CellClient_Close(NativeTCPClient* pClient) {
		if (pClient) {
			pClient->Close();
			delete pClient;
			pClient = nullptr;
		}
	}

	EXPORT_DLL int CellClient_SendData(NativeTCPClient* pClient, const char* data, int len) {
		if (pClient) {
			return pClient->SendData(data, len);
		}

		return 0;
	}

	EXPORT_DLL int CellClient_SendWriteStream(NativeTCPClient* pClient, CellWriteStream* wStream) {
		if (pClient && wStream) {
			wStream->finish();
			return pClient->SendData(wStream->data(), wStream->length());
		}

		return 0;
	}

	//////////////////CellMsgStream
	///////// CellWriteStream
	EXPORT_DLL void* CellWriteStream_Create(int nSize) {
		CellWriteStream* wStream = new CellWriteStream(nSize);
		return wStream;
	}

	EXPORT_DLL bool CellWriteStream_WriteInt8(CellWriteStream* wStream, int8_t n) {
		if (wStream) {
			return wStream->write(n);
		}
		return false;
	}

	EXPORT_DLL bool CellWriteStream_WriteInt16(CellWriteStream* wStream, int16_t n) {
		if (wStream) {
			return wStream->write(n);
		}
		return false;
	}

	EXPORT_DLL bool CellWriteStream_WriteInt32(CellWriteStream* wStream, int32_t n) {
		if (wStream) {
			return wStream->write(n);
		}
		return false;
	}

	EXPORT_DLL bool CellWriteStream_WriteInt64(CellWriteStream* wStream, int64_t n) {
		if (wStream) {
			return wStream->write(n);
		}
		return false;
	}

	EXPORT_DLL bool CellWriteStream_WriteUInt8(CellWriteStream* wStream, uint8_t n) {
		if (wStream) {
			return wStream->write(n);
		}
		return false;
	}

	EXPORT_DLL bool CellWriteStream_WriteUInt16(CellWriteStream* wStream, uint16_t n) {
		if (wStream) {
			return wStream->write(n);
		}
		return false;
	}

	EXPORT_DLL bool CellWriteStream_WriteUInt32(CellWriteStream* wStream, uint32_t n) {
		if (wStream) {
			return wStream->write(n);
		}
		return false;
	}

	EXPORT_DLL bool CellWriteStream_WriteUInt64(CellWriteStream* wStream, uint64_t n) {
		if (wStream) {
			return wStream->write(n);
		}
		return false;
	}

	EXPORT_DLL bool CellWriteStream_WriteFloat(CellWriteStream* wStream, float n) {
		if (wStream) {
			return wStream->write(n);
		}
		return false;
	}

	EXPORT_DLL bool CellWriteStream_WriteDouble(CellWriteStream* wStream, double n) {
		if (wStream) {
			return wStream->write(n);
		}
		return false;
	}

	EXPORT_DLL bool CellWriteStream_WriteString(CellWriteStream* wStream, char* n) {
		if (wStream) {
			return wStream->writeString(n);
		}
		return false;
	}

	EXPORT_DLL void CellWriteStream_Release(CellWriteStream* wStream) {
		if (wStream) {
			delete wStream;
		}
	}
	/////CellReadStream
	EXPORT_DLL void* CellReadStream_Create(char* data, int len) {
		CellReadStream* rStream = new CellReadStream(data, len);
		return rStream;
	}

	EXPORT_DLL int8_t CellReadStream_ReadInt8(CellReadStream* rStream) {
		if (rStream) {
			return rStream->readInt8();
		}
		return 0;
	}

	EXPORT_DLL int16_t CellReadStream_ReadInt16(CellReadStream* rStream) {
		if (rStream) {
			return rStream->readInt16();
		}
		return 0;
	}

	EXPORT_DLL int32_t CellReadStream_ReadInt32(CellReadStream* rStream) {
		if (rStream) {
			return rStream->readInt32();
		}
		return 0;
	}

	EXPORT_DLL int64_t CellReadStream_ReadInt64(CellReadStream* rStream) {
		if (rStream) {
			return rStream->readInt64();
		}
		return 0;
	}

	EXPORT_DLL uint8_t CellReadStream_ReadUInt8(CellReadStream* rStream) {
		if (rStream) {
			return rStream->readUInt8();
		}
		return 0;
	}

	EXPORT_DLL uint16_t CellReadStream_ReadUInt16(CellReadStream* rStream) {
		if (rStream) {
			return rStream->readUInt16();
		}
		return 0;
	}

	EXPORT_DLL uint32_t CellReadStream_ReadUInt32(CellReadStream* rStream) {
		if (rStream) {
			return rStream->readUInt32();
		}
		return 0;
	}

	EXPORT_DLL uint64_t CellReadStream_ReadUInt64(CellReadStream* rStream) {
		if (rStream) {
			return rStream->readUInt64();
		}
		return 0;
	}

	EXPORT_DLL float CellReadStream_ReadFloat(CellReadStream* rStream) {
		if (rStream) {
			return rStream->readFloat();
		}
		return 0.0f;
	}

	EXPORT_DLL double CellReadStream_ReadDouble(CellReadStream* rStream) {
		if (rStream) {
			return rStream->readDouble();
		}
		return 0.0;
	}

	EXPORT_DLL bool CellReadStream_ReadString(CellReadStream* rStream, char* buffer, int len) {
		if (rStream && buffer) {
			return rStream->readArray(buffer, len);
		}
		return false;
	}

	EXPORT_DLL uint32_t CellReadStream_OnlyReadUint32(CellReadStream* rStream) {
		uint32_t len = 0;
		if (rStream) {
			rStream->onlyRead(len);
		}
		return len;
	}

	EXPORT_DLL void CellReadStream_Release(CellReadStream* rStream) {
		if (rStream) {
			delete rStream;
		}
	}
}

#endif // !_CPP_NET_100_DLL_H_
