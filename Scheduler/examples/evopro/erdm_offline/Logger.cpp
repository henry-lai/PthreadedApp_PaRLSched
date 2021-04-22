/*
 * evopro Innovation Kft.
 *
 * sni
 *
 * File: Logger.cpp
 * <Description>
 * Created on: Jul 28, 2014
 *
 * Author: pechan
 * (C) 2014. evopro Innovation Kft.
 *
 */

#include "Logger.h"

namespace sniutils {

Logger::Logger() {

	//opening log with the specified ident, with PID appended and as daemon
	openlog(LOG_IDENT, LOG_PID, LOG_DAEMON);

	//DEBUG code - writing also to external file
	//ofstream measFile;
	//measFile.open("/usr/local/sni/log.txt", ios_base::out | ios_base::trunc);
	//measFile << "New session started" << endl;
	//measFile.close();
}

Logger::~Logger() {

	//closing log
	closelog();
}

void Logger::addLocInfo(int lineNum, const char* funcName) {

	Logger::getL() << "In " << funcName << " at " << lineNum << ": ";
}

void Logger::writeLog(int priority) {

	string logMsg;

	//write current stringstream content to log with the specified priority
	syslog(priority, Logger::getL().str().c_str());

	//DEBUG code - writing also to external file
	//ofstream measFile;
	//measFile.open("/usr/local/sni/log.txt", ios_base::out | ios_base::app);
	//measFile << Logger::getL().str() << endl;
	//measFile.close();

	//clear the stringstream object
	Logger::getL().str("");
}

} /* namespace sniutils */


