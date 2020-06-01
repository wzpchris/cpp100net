#ifndef _CELL_LOG_HPP_
#define _CELL_LOG_HPP_

#include "Cell.hpp"
#include "CellTask.hpp"
#include <stdio.h>
#include <ctime>

class CellLog {
	//Info
	//Debug
	//Waring
	//Error
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

	void setLogPath(const char* logPath, const char* mode) {
		if (_logFile) {
			Info("CellLog::setLogPath _logFile != nullptr\n");
			fclose(_logFile);
			_logFile = nullptr;
		}

		_logFile = fopen(logPath, mode);
		if (_logFile) {
			Info("CellLog::setLogPath success,<%s:%s>\n", logPath, mode);
		}
		else {
			Info("CellLog::setLogPath failed,<%s:%s>\n", logPath, mode);
		}
	}

	static void Info(const char* pStr) {
		CellLog* pLog = &Instance();
		pLog->_taskServer.addTask([=]() {
			if (pLog->_logFile) {
				auto t = std::chrono::system_clock::now();
				auto tNow = std::chrono::system_clock::to_time_t(t);
				//fprintf(pLog->_logFile, "%s", ctime(&tNow));
				std::tm* now = std::gmtime(&tNow);
				fprintf(pLog->_logFile, "[%04d-%02d-%02d %02d:%02d:%02d] ",
					now->tm_year + 1900,
					now->tm_mon + 1,
					now->tm_mday,
					now->tm_hour + 8,
					now->tm_min,
					now->tm_sec);
				fprintf(pLog->_logFile, "%s", pStr);
				fflush(pLog->_logFile);
			}
			printf("%s", pStr);
		});
	}

	template<typename ...Args>
	static void Info(const char* pFormat, Args ... args) {
		CellLog* pLog = &Instance();
		pLog->_taskServer.addTask([=]() {
			if (pLog->_logFile) {
				auto t = std::chrono::system_clock::now();
				auto tNow = std::chrono::system_clock::to_time_t(t);
				//fprintf(pLog->_logFile, "%s", ctime(&tNow));
				std::tm* now = std::gmtime(&tNow);
				fprintf(pLog->_logFile, "[%04d-%02d-%02d %02d:%02d:%02d] ",
					now->tm_year + 1900,
					now->tm_mon + 1,
					now->tm_mday,
					now->tm_hour + 8,
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
