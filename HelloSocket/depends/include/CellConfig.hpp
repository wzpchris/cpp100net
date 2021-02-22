#ifndef _CELL_CONFIG_HPP_
#define _CELL_CONFIG_HPP_

#include <string>
#include "CellLog.hpp"
#include <map>

/// <summary>
/// 专门用于读取配置数据
/// 目前主要来源于main函数的args传入
/// </summary>

class CellConfig {
private:
	CellConfig() {
	}
	~CellConfig() {

	}
public:
	static CellConfig& Instance() {
		static CellConfig obj;
		return obj;
	}

	void Init(int argc, char* args[]) {
		_exePath = args[0];
		for (int n = 1; n < argc; ++n) {
			makeCmd(args[n]);
		}
	}

	void makeCmd(char* cmd) {
		//cmd值:strIP=127.0.0.1
		char* val = strchr(cmd, '=');
		if (val != nullptr) {
			*val = '\0';
			//cmd值:strIP\0127.0.0.1
			//val值:\0127.0.0.1
			val++;
			//val值:127.0.0.1
			_kv[cmd] = val;
			CellLog_Info("makeCmd %s|%s\n", cmd, val);
		}
		else {
			_kv[cmd] = "";
			CellLog_Info("makeCmd %s\n", cmd);
		}
	}

	const char* getStr(const char* argName, const char* def) {
		auto iter = _kv.find(argName);
		if (iter == _kv.end()) {
			CellLog_Error("CellConfig::getStr not find argName=%s\n", argName);
			return def;
		}

		CellLog_Info("CellConfig::getStr find %s|%s\n", argName, iter->second.c_str());
		return iter->second.c_str();
	}

	int getInt(const char* argName, int def) {
		auto iter = _kv.find(argName);
		if (iter == _kv.end()) {
			CellLog_Error("CellConfig::getStr not find argName=%s\n", argName);
			return def;
		}

		CellLog_Info("CellConfig::getStr find %s|%s\n", argName, iter->second.c_str());
		return atoi(iter->second.c_str());
	}

	bool hasKey(const char* argName) {
		return _kv.find(argName) != _kv.end();
	}

private:
	/// <summary>
	/// 当前程序的路径
	/// </summary>
	std::string _exePath;
	/// <summary>
	/// 存储传入的key-val型数据
	/// </summary>
	std::map<std::string, std::string> _kv;
};
#endif // !_CELL_CONFIG_HPP_
