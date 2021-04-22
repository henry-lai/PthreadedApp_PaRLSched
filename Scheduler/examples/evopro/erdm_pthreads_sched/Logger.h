/*
 * evopro Innovation Kft.
 *
 * sni
 *
 * File: Logger.h
 * <Description>
 * Created on: Jul 28, 2014
 *
 * Author: pechan
 * (C) 2014. evopro Innovation Kft.
 *
 */

#ifndef ERDM_PC_LOGGER_H_
#define ERDM_PC_LOGGER_H_

#include <sstream>

extern "C" {
#include <syslog.h>
}

//program name to be included in log
//TODO: should be set with external parameter for reusability...
#define LOG_IDENT "sni"

//should be used as Logger::getL() << LOG_LOCINFO;
#define LOG_LOCINFO "In " << __func__ << " at " << __LINE__ << ": "

using namespace std;

namespace sniutils {

//singleton class for accessing system log
//appending something to log message: Logger::getL() << "message"
//writing current log message to system log: Logger::writeAsDebug() and similar functions
//appending location info: Logger::getL() << LOG_LOCINFO; or Logger::addLocInfo(__LINE__, __func__);
class Logger: public stringstream {
public:
	static Logger& getL() {
		static Logger logStr;
		return logStr;
	}
	static void writeAsDebug() {
		Logger::writeLog(LOG_DEBUG);
	}
	static void writeAsInfo() {
		Logger::writeLog(LOG_INFO);
	}
	static void writeAsError() {
		Logger::writeLog(LOG_ERR);
	}
	//should be called as Logger::addLocInfo(__LINE__, __func__);
	static void addLocInfo(int lineNum, const char* funcName);
private:
	Logger();
	Logger(const Logger& other);
	Logger& operator=(const Logger& other);
	~Logger();
	static void writeLog(int priority);
};

} /* namespace sniutils */
#endif /* ERDM_PC_LOGGER_H_ */
