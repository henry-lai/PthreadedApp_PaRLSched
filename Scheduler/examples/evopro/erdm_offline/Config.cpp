/*
 * evopro Innovation Kft.
 *
 * sni
 *
 * File: Config.cpp
 * <Description>
 * Created on: Aug 15, 2014
 *
 * Author: pechan
 * (C) 2014. evopro Innovation Kft.
 *
 */

#include "Config.h"

namespace sniutils {

const char* const Config::gwIdFileName = "/etc/hostname";
const char* const Config::serialPortName = "/dev/rs485";
//const char* const Config::serialPortName = "/dev/ttyUSB1";
const speed_t Config::serialPortSpeed = B921600;
const unsigned int Config::serialCharSize = 8;
const bool Config::serialParityEn = false;
const unsigned int Config::serialNumStop = 1;
const unsigned int Config::serialReconnectToSec = 3;

const char* const Config::measSocketName = "/tmp/meas_data";
const char* const Config::diagSocketName = "/tmp/sni_diag";
const char* const Config::busPowerSocketName = "/tmp/buspower";

const std::string Config::configFileName("/data/settings.json");
const std::string Config::measFilesDirName("/data/measdata");
//const std::string Config::configFileName("/home/pechan/Work/data/settings_testpad.json");
//const std::string Config::measFilesDirName("/home/pechan/Work/data/measdata");
const off_t Config::measFileStorageBytes = 32*1024*1024;

const unsigned int Config::numMaxAxles = 1024;

const bool Config::isDefDirLow2Up = true;
const float Config::defAxSpeed_kmph = 60.0f;
const float Config::defTemperature_C = 20.0f;

const unsigned int Config::stateCheckPeriodSec = 60;
const unsigned int Config::rsbRearmPeriod = 15;
const unsigned int Config::minMeasDelayToArmSec = 3;

} /* namespace sniutils */
