#include "EasyTcpClient.hpp"
#include "CellStream.hpp"
#include "CellMsgStream.hpp"
#include <iostream>

class MyClient :public EasyTcpClient {
public:
	virtual void OnNetMsg(netmsg_DataHeader* header) {
		//6.处理请求
		switch (header->cmd)
		{
		case CMD_LOGOUT_RESULT:
		{
			CellReadStream r(header);
			auto n1 = r.readInt8();
			auto n2 = r.readInt16();
			auto n3 = r.readInt32();
			auto n4 = r.readFloat();
			auto n5 = r.readDouble();

			uint32_t n = 0;
			r.onlyRead(n);

			char name[32] = {};
			auto n6 = r.readArray(name, 32);
			char pw[32] = {};
			auto n7 = r.readArray(pw, 32);

			int data[10] = {};
			auto n8 = r.readArray(data, 10);

			//netmsg_LoginR *loginret = (netmsg_LoginR*)header;
			/*CellLog::Info("recv server CMD_LOGIN_RESULT msg: [len=%d, cmd=%d, result=%d]\n",
				loginret->dataLength, loginret->cmd, loginret->result);*/
		}
		break;
		case CMD_ERROR:
		{
			CellLog::Info("recv error msg len=%d...\n", header->dataLength);
		}
		break;
		default: {
			CellLog::Info("recv unkown msglen=%d...\n", header->dataLength);
		}
		};
	}
};


int main() {
	CellLog::Instance().setLogPath("socketStream.log", "w");
	CellWriteStream s;
	s.setNetCmd(CMD_LOGOUT);
	s.writeInt8(1);
	s.writeInt16(2);
	s.writeInt32(3);
	s.writeFloat(4.5f);
	s.writeDouble(6.7);
	const char* str = "client";
	s.writeString(str);
	char a[] = "ahah";
	s.writeArray(a, strlen(a));
	int b[] = { 1, 2, 3, 4, 5 };
	s.writeArray(b, 5);
	s.finish();

	MyClient client;
	client.Connect("127.0.0.1", 4567);
	client.SendData(s.data(), s.length());
	while (client.IsRun()) {
		client.OnRun();
		CellThread::Sleep(10);
	}
	return 0;
}
