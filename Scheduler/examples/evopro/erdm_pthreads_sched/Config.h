/*
 * evopro Innovation Kft.
 *
 * sni
 *
 * File: Config.h
 * <Description>
 * Created on: Aug 15, 2014
 *
 * Author: pechan
 * (C) 2014. evopro Innovation Kft.
 *
 */

#ifndef ERDM_PC_CONFIG_H_
#define ERDM_PC_CONFIG_H_

#include <string>

#include <termios.h>
#include <sys/types.h>

namespace sniutils {

class Config {
public:
	//serial port name and parameters
	static const char* const gwIdFileName;
	static const char* const serialPortName;
	static const speed_t serialPortSpeed;
	static const unsigned int serialCharSize;
	static const bool serialParityEn;
	static const unsigned int serialNumStop;
	static const unsigned int serialReconnectToSec;
	//unix sockets
	static const char* const measSocketName;
	static const char* const diagSocketName;
	static const char* const busPowerSocketName;
	//path of configuration file
	static const std::string configFileName;
	static const std::string measFilesDirName;
	static const off_t measFileStorageBytes;
	//limits
	static const unsigned int numMaxAxles;
	//default values
	static const bool isDefDirLow2Up;
	static const float defAxSpeed_kmph;
	static const float defTemperature_C;
	//other parameters
	static const unsigned int stateCheckPeriodSec;
	static const unsigned int rsbRearmPeriod;
	static const unsigned int minMeasDelayToArmSec;
private:
	Config();
};

} /* namespace sniutils */
#endif /* ERDM_PC_CONFIG_H_ */
