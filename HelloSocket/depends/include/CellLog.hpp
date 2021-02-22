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
#define CellLog_PError(...) CellLog::PError(__VA_ARGS__)

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

	void setLogPath(const char* logName, const char* mode, bool hasDate) {
		if (_logFile) {
			Info("CellLog::setLogPath _logFile != nullptr\n");
			fclose(_logFile);
			_logFile = nullptr;
		}
		//静态变量
		static char logPath[256] = {};
		if (hasDate) {
			auto t = std::chrono::system_clock::now();
			auto tNow = std::chrono::system_clock::to_time_t(t);
			std::tm* now = std::localtime(&tNow);
			sprintf(logPath, "%s.%04d-%02d-%02d_%02d-%02d-%02d",
				logName,
				now->tm_year + 1900,
				now->tm_mon + 1,
				now->tm_mday,
				now->tm_hour,
				now->tm_min,
				now->tm_sec);
		}
		else {
			sprintf(logPath, "%s", logName);
		}

		_logFile = fopen(logPath, mode);
		if (_logFile) {
			Info("CellLog::setLogPath success,<%s:%s>\n", logPath, mode);
		}
		else {
			Info("CellLog::setLogPath failed,<%s:%s>\n", logPath, mode);
		}
	}

	static void PError(const char* pStr) {
		PError("%s", pStr);
	}

	template<typename ...Args>
	static void PError(const char* pFormat, Args ... args) {
#ifdef _WIN32
		auto errCode = GetLastError();
		
		/*char* text = nullptr;
		FormatMessageA(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			errCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&text,0,NULL);
		Echo("PError", "errno<%d>, errmsg<%s>\n", errCode, text);
		CellLog* pLog = &Instance();
		pLog->_taskServer.addTask([=]() {
			LocalFree(text);
		});
		*/

		CellLog* pLog = &Instance();
		pLog->_taskServer.addTask([=]() {
			char text[256] = {};
			FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				errCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPSTR)&text, 256, NULL
			);
			EchoReal("PError", pFormat, args...);
			EchoReal("PError", "errno:%d, errmsg:%s", errCode, text);
		});
#else
		auto errCode = errno;
		CellLog* pLog = &Instance();
		pLog->_taskServer.addTask([=]() {
			Echo("PError", pFormat, args...);
			Echo("PError", "errno:%d, errmsg:%s", errCode, strerror(errCode));
		});
#endif
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
			EchoReal(type, pFormat, args...);
		});
	}

	template<typename ...Args>
	static void EchoReal(const char* type, const char* pFormat, Args ... args) {
		CellLog* pLog = &Instance();
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
	}

private:
	FILE* _logFile = nullptr;
	CellTaskServer _taskServer;
};
#endif // !_CELL_LOG_HPP_
