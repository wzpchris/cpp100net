#ifndef _CELL_LOG_HPP_
#define _CELL_LOG_HPP_

#include "Cell.hpp"
#include "CellTask.hpp"
#include <stdio.h>
#include <ctime>

//Info 普通信息
//Debug 调试信息，只在debug模式起作用
//Waring 警告信息
//Error 错误信息
class CellLog {

#ifdef  _DEBUG
#ifndef CellLog_Debug
#define CellLog_Debug(...) CellLog::Debug(__VA_ARGS__)
#endif
#else
#ifndef CellLog_Debug
#define CellLog_Debug(...)
#endif
#endif //  _DEBUG

#define CellLog_Info(...) CellLog::Info(__VA_ARGS__)
#define CellLog_Waring(...) CellLog::Waring(__VA_ARGS__)
#define CellLog_Error(...) CellLog::Error(__VA_ARGS__)

private:
	CellLog() {
		_taskServer.Start();
	}
	~CellLog() {
		_taskServer.Close();
		if (_logFile) {
			Info("CellLog fclose(_logFile)\n");
			fclose(_logFile);
			_logFile = nullptr;
		}
	}

public:
	static CellLog& Instance() {
		static CellLog sLog;
		return sLog;
	}

	void setLogPath(const char* logName, const char* mode) {
		if (_logFile) {
			Info("CellLog::setLogPath _logFile != nullptr\n");
			fclose(_logFile);
			_logFile = nullptr;
		}
		//静态变量
		static char logPath[256] = {};
		auto t = std::chrono::system_clock::now();
		auto tNow = std::chrono::system_clock::to_time_t(t);
		std::tm* now = std::localtime(&tNow);
		sprintf(logPath, "%s.%04d-%02d-%02d",
			logName,
			now->tm_year + 1900,
			now->tm_mon + 1,
			now->tm_mday);

		_logFile = fopen(logPath, mode);
		if (_logFile) {
			Info("CellLog::setLogPath success,<%s:%s>\n", logPath, mode);
		}
		else {
			Info("CellLog::setLogPath failed,<%s:%s>\n", logPath, mode);
		}
	}

	static void Error(const char* pStr) {
		Error("%s", pStr);
	}

	template<typename ...Args>
	static void Error(const char* pFormat, Args ... args) {
		Echo("Error", pFormat, args...);
	}

	static void Waring(const char* pStr) {
		Waring("%s", pStr);
	}

	template<typename ...Args>
	static void Waring(const char* pFormat, Args ... args) {
		Echo("Waring", pFormat, args...);
	}

	static void Debug(const char* pStr) {
		Debug("%s", pStr);
	}

	template<typename ...Args>
	static void Debug(const char* pFormat, Args ... args) {
		Echo("Debug", pFormat, args...);
	}

	static void Info(const char* pStr) {
		Info("%s", pStr);
	}

	template<typename ...Args>
	static void Info(const char* pFormat, Args ... args) {
		Echo("Info", pFormat, args...);
	}

	template<typename ...Args>
	static void Echo(const char* type, const char* pFormat, Args ... args) {
		CellLog* pLog = &Instance();
		pLog->_taskServer.addTask([=]() {
			if (pLog->_logFile) {
				auto t = std::chrono::system_clock::now();
				auto tNow = std::chrono::system_clock::to_time_t(t);
				fprintf(pLog->_logFile, "%s", type);
				std::tm* now = std::localtime(&tNow);
				fprintf(pLog->_logFile, " [%04d-%02d-%02d %02d:%02d:%02d] ",
					now->tm_year + 1900,
					now->tm_mon + 1,
					now->tm_mday,
					now->tm_hour,
					now->tm_min,
					now->tm_sec);
				fprintf(pLog->_logFile, pFormat, args...);
				fflush(pLog->_logFile);
			}
			printf(pFormat, args...);
			});
	}

private:
	FILE* _logFile = nullptr;
	CellTaskServer _taskServer;
};
#endif // !_CELL_LOG_HPP_
