#ifndef _MESSAGE_HEADER_HPP_
#define _MESSAGE_HEADER_HPP_

enum CMD {
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_NEW_USER_JOIN,
	CMD_C2S_HEART,
	CMD_S2C_HEART,
	CMD_ERROR,
};

//消息头
struct netmsg_DataHeader {
	netmsg_DataHeader() {
		dataLength = sizeof(netmsg_DataHeader);
		cmd = CMD_ERROR;
	}
	short dataLength;
	short cmd;
};

//DataPackage
struct netmsg_Login : public netmsg_DataHeader {
	netmsg_Login() {
		dataLength = sizeof(netmsg_Login);
		cmd = CMD_LOGIN;
	}
	char UserName[32];
	char PassWord[32];
	char data[32];
};

struct netmsg_LoginR :public netmsg_DataHeader {
	netmsg_LoginR() {
		dataLength = sizeof(netmsg_LoginR);
		cmd = CMD_LOGIN_RESULT;
		result = 1;
	}
	int result;
	char data[92];
};

struct netmsg_LogOut :public netmsg_DataHeader {
	netmsg_LogOut() {
		dataLength = sizeof(netmsg_LogOut);
		cmd = CMD_LOGOUT;
	}
	char UserName[32];
};

struct netmsg_LogOutR :public netmsg_DataHeader {
	netmsg_LogOutR() {
		dataLength = sizeof(netmsg_LogOutR);
		cmd = CMD_LOGOUT_RESULT;
		result = 1;
	}
	int result;
};

struct netmsg_NewUserJoin :public netmsg_DataHeader {
	netmsg_NewUserJoin() {
		dataLength = sizeof(netmsg_NewUserJoin);
		cmd = CMD_NEW_USER_JOIN;
		sock = 0;
	}
	int sock;
};

struct netmsg_c2s_Heart :public netmsg_DataHeader {
	netmsg_c2s_Heart() {
		dataLength = sizeof(netmsg_c2s_Heart);
		cmd = CMD_C2S_HEART;
	}
};

struct netmsg_s2c_Heart :public netmsg_DataHeader {
	netmsg_s2c_Heart() {
		dataLength = sizeof(netmsg_s2c_Heart);
		cmd = CMD_S2C_HEART;
	}
};
#endif 