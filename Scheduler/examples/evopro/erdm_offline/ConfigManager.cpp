/*
 * evopro Innovation Kft.
 *
 * sni
 *
 * File: ConfigManager.cpp
 * <Description>
 * Created on: Aug 13, 2014
 *
 * Author: pechan
 * (C) 2014. evopro Innovation Kft.
 *
 */

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

#include "ConfigManager.h"
#include "Utils.h"

namespace sni {

const unsigned int ConfigLimits::minNumRcb = 6;
const unsigned int ConfigLimits::minNumRsbAtBorder = 1;
const unsigned int ConfigLimits::maxNumRcbSens = 2;
const float ConfigLimits::minGlobalScale = 0.5;
const float ConfigLimits::maxGlobalScale = 1.5;
const unsigned int ConfigLimits::numValidNumSamples = 2;
const unsigned int ConfigLimits::validNumSamples[numValidNumSamples] = {32, 64};

const uint16_t SensorParameters::comErrorResetMask 			= 0x0001;
const uint16_t SensorParameters::comErrorEchoMask 			= 0x0002;
const uint16_t SensorParameters::comErrorAutoZeroMask 		= 0x0004;
const uint16_t SensorParameters::statErrorAutoZeroMask 		= 0x0008;
const uint16_t SensorParameters::comErrorArmMask 			= 0x0010;
const uint16_t SensorParameters::statErrorArmMask 			= 0x0020;
const uint16_t SensorParameters::comErrorCheckStateMask 	= 0x0040;
const uint16_t SensorParameters::statErrorNotArmedMask 		= 0x0080;
const uint16_t SensorParameters::comErrorWhileMeasMask 		= 0x0100;
const uint16_t SensorParameters::comErrorDisarmMask 		= 0x0200;
const uint16_t SensorParameters::statErrorDisarmMask 		= 0x0400;
const uint16_t SensorParameters::comErrorGetAxcntMask 		= 0x0800;
const uint16_t SensorParameters::comErrorGetMeasMask 		= 0x1000;
const uint16_t SensorParameters::rsbErrorFailureStateMask	= 0x2000;

//////////////////////////////////////////////////////////
//		function definitions for class SensorParameters
//////////////////////////////////////////////////////////

SensorParameters::SensorParameters(const json::Object& jsonObj) :
	alive(true) {

	json::Object::ValueMap::const_iterator confDataIt;
	string addrStr;
	stringstream convStream;
	unsigned int addrTemp;

	//get and convert address field
	addrStr = json::getStringJsonField(jsonObj, "address");
	if ((addrStr.size() != 4) || (addrStr[0] != '0') || ((addrStr[1] != 'x') && (addrStr[1] != 'X'))) {
		throw runtime_error("Invalid address format");
	}
	convStream << hex << addrStr.substr(2,2);
	convStream >> addrTemp;
	if ((convStream.fail() == true) || (addrTemp > 255)) {
		throw runtime_error("Invalid address format");
	}
	address = sniutils::Utils::getIntByte<unsigned int>(addrTemp, 0);

	//get tempSensor field
	tempSensor = json::getBoolJsonField(jsonObj, "tempSensor");

	idValid = false;
	ID = 0;
	statusBits = 0;
}

char SensorParameters::getAddress() const {
	return address;
}

uint32_t SensorParameters::getId() const {
	return ID;
}

void SensorParameters::setId(uint32_t newId) {
	idValid = true;
	ID = newId;
}

bool SensorParameters::isIdValid() const {
	return idValid;
}

bool SensorParameters::isTempSensor() const {
	return tempSensor;
}

bool SensorParameters::operator==(const SensorParameters& rhs) const {

	//compare members pairwise
	if (address != rhs.address) return false;
	if (tempSensor != rhs.tempSensor) return false;

	return true;
}

bool SensorParameters::operator!=(const SensorParameters& rhs) const {

	if ((*this) == rhs) {
		return false;
	}
	return true;
}

bool SensorParameters::isAlive() const {
	return alive;
}

void SensorParameters::setAlive(bool alive) {
	this->alive = alive;
}

void SensorParameters::resetStatusBits() {
	statusBits = 0;
}

uint16_t SensorParameters::getStatusBits() const {
	return statusBits;
}

void SensorParameters::addStatusBitMask(uint16_t statMask) {
	statusBits |= statMask;
}

//////////////////////////////////////////////////////////
//		function definitions for class RsbParameters
//////////////////////////////////////////////////////////

RsbParameters::RsbParameters(const json::Object& jsonObj) :
		SensorParsWithMeasData(jsonObj) {

	unsigned int posIdx;

	json::Array posArray = json::getArrayJsonField(jsonObj, "sensPositions_mm");
	for (posIdx = 0; posIdx < posArray.size(); posIdx++) {
		sensPositions_mm.push_back(posArray[posIdx]);
	}

	failureState = false;
}

const vector<int>& RsbParameters::RsbParameters::getSensPositions_mm() const {
	return sensPositions_mm;
}

void RsbParameters::setSensAtUpperEnd(int lowestRcbPos_mm, int highestRcbPos_mm) {

	unsigned int sensIdx;

	for (sensIdx = 0; sensIdx < sensPositions_mm.size(); sensIdx++) {
		if (sensPositions_mm[sensIdx] < lowestRcbPos_mm) {
			sensAtUpperEnd.push_back(false);
		}
		else {
			if (sensPositions_mm[sensIdx] > highestRcbPos_mm) {
				sensAtUpperEnd.push_back(true);
			}
			else {
				throw runtime_error("Error: Invalid RSB position");
			}
		}
	}
}

const vector<bool>& RsbParameters::isSensAtUpperEnd() const {
	return sensAtUpperEnd;
}

unsigned int RsbParameters::getNumSens() const {
	return sensPositions_mm.size();
}

bool RsbParameters::isSensAtUpperEnd(unsigned int sensIdx) const {
	return sensAtUpperEnd[sensIdx];
}

bool RsbParameters::operator==(const RsbParameters& rhs) const {

	if (SensorParameters::operator==(rhs) != true) return false;
	if (sensPositions_mm != rhs.sensPositions_mm) return false;
	if (sensAtUpperEnd != rhs.sensAtUpperEnd) return false;

	return true;
}

bool RsbParameters::operator!=(const RsbParameters& rhs) const {

	if ((*this) == rhs) {
		return false;
	}
	return true;
}

vector<RsbMeasLdataAnswData>& RsbParameters::getMeasData() {
	return measData;
}

unsigned int RsbParameters::getNumMeas() const {
	return measData.size();
}
MeasLdataAnswData& RsbParameters::getSingleMeas(unsigned int measIdx) {
	return measData[measIdx];
}

RsbMeasLdataAnswData& RsbParameters::getSingleMeasFull(unsigned int measIdx) {
	return measData[measIdx];
}

void RsbParameters::setNumMeasDummy(unsigned int numMeas) {

	measData.clear();
	measData.resize(numMeas);
}

void RsbParameters::setMeasFromJson(const json::Object& measObj, unsigned int measIdx) {

	measData[measIdx].fromJsonObj(measObj);
}

void RsbParameters::setFailureState() {
	failureState = true;
}

void RsbParameters::resetFailureState() {
	failureState = false;
}

bool RsbParameters::isFailureState() const {
	return failureState;
}

//////////////////////////////////////////////////////////
//		function definitions for class RsbParameters
//////////////////////////////////////////////////////////

RcbParameters::RcbParameters(const json::Object& jsonObj) :
		SensorParsWithMeasData(jsonObj) {

	unsigned int dirIdx;

	//get each field
	onLeftRail = json::getBoolJsonField(jsonObj, "onLeftRail");
	position_mm = json::getIntJsonField(jsonObj, "position_mm");

	json::Array scaleOffsetArray = json::getArrayJsonField(jsonObj, "scaleOffset");
	json::Array scaleTempArray = json::getArrayJsonField(jsonObj, "scaleTemp");
	json::Array scaleSpeedArray = json::getArrayJsonField(jsonObj, "scaleSpeed");
	if ((scaleOffsetArray.size() != 2) || (scaleTempArray.size() != 2) || (scaleSpeedArray.size() != 2)) {
		throw runtime_error("RCB scale arrays invalid");
	}

	for (dirIdx = 0; dirIdx < 2; dirIdx++) {
		scaleOffset[dirIdx] = scaleOffsetArray[dirIdx].ToFloat();
		scaleTemp[dirIdx] = scaleTempArray[dirIdx].ToFloat();
		scaleSpeed[dirIdx] = scaleSpeedArray[dirIdx].ToFloat();
	}
}

bool RcbParameters::isOnLeftRail() const {
	return onLeftRail;
}

int RcbParameters::getPosition_mm() const {
	return position_mm;
}

float RcbParameters::getScaleOffset(bool dirFromLowerToUpper) const {
	return dirFromLowerToUpper ? scaleOffset[0] : scaleOffset[1];
}

float RcbParameters::getScaleSpeed(bool dirFromLowerToUpper) const {
	return dirFromLowerToUpper ? scaleSpeed[0] : scaleSpeed[1];
}

float RcbParameters::getScaleTemp(bool dirFromLowerToUpper) const {
	return dirFromLowerToUpper ? scaleTemp[0] : scaleTemp[1];
}

bool RcbParameters::operator==(const RcbParameters& rhs) const {

	unsigned int dirIdx;

	if (SensorParameters::operator==(rhs) != true) return false;
	if (position_mm != rhs.position_mm) return false;
	if (onLeftRail != rhs.onLeftRail) return false;
	for (dirIdx = 0; dirIdx < 2; dirIdx++) {
		if (scaleOffset[dirIdx] != rhs.scaleOffset[dirIdx]) return false;
		if (scaleTemp[dirIdx] != rhs.scaleTemp[dirIdx]) return false;
		if (scaleSpeed[dirIdx] != rhs.scaleSpeed[dirIdx]) return false;
	}

	return true;
}

bool RcbParameters::operator!=(const RcbParameters& rhs) const {

	if ((*this) == rhs) {
		return false;
	}
	return true;
}

vector<RcbMeasLdataAnswData>& RcbParameters::getMeasData() {
	return measData;
}

unsigned int RcbParameters::getNumMeas() const {
	return measData.size();
}

MeasLdataAnswData& RcbParameters::getSingleMeas(unsigned int measIdx) {
	return measData[measIdx];
}

RcbMeasLdataAnswData& RcbParameters::getSingleMeasFull(unsigned int measIdx) {
	return measData[measIdx];
}

void RcbParameters::setNumMeasDummy(unsigned int numMeas) {

	measData.clear();
	measData.resize(numMeas);
}

void RcbParameters::setMeasFromJson(const json::Object& measObj, unsigned int measIdx) {

	measData[measIdx].fromJsonObj(measObj);
}

//////////////////////////////////////////////////////////
//		function definitions for class SniParameters
//////////////////////////////////////////////////////////

SniParameters::SniParameters() :
	globalScale(0.0f),
	numSamples(32) {

}

SniParameters::SniParameters(const json::Object& jsonObj) {

	//get each field
	globalScale = json::getFloatJsonField(jsonObj, "globalScale");
	numSamples = (unsigned int) json::getIntJsonField(jsonObj, "numSamples");
}

float SniParameters::getGlobalScale() const {
	return globalScale;
}

unsigned int SniParameters::getNumSamples() const {
	return numSamples;
}

bool SniParameters::operator==(const SniParameters& rhs) const {

	if (globalScale != rhs.globalScale) return false;
	if (numSamples != rhs.numSamples) return false;

	return true;
}

bool SniParameters::operator!=(const SniParameters& rhs) const {

	if ((*this) == rhs) {
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////
//		function definitions for class ConfigManager
//////////////////////////////////////////////////////////

ConfigManager::ConfigManager(Logger& sysLogInit, const string& jsonFileNameInit, string offlineMeasFileName):
	sysLog(sysLogInit),
	jsonFileName(jsonFileNameInit) {

	//parse configuration file and validate parameters
	try {
		parseJsonFile(rsbData, rcbData, sniPars);
		validateParameters(rsbData, rcbData, sniPars, &numRsbSensAtUpperEnd, &numRsbSensAtLowerEnd);
	}
	catch (...) {
		sysLog << "Error: cannot initialize configuration parameters from JSON file";
		sysLog.writeAsError();
		throw;
	}

	//set reference arrays
	setRefArrays();

	//process measurement file if provided
	if (offlineMeasFileName.empty() == false) {
		addMeasFile(offlineMeasFileName);
	}
}

ConfigManager::~ConfigManager() {

}

void ConfigManager::updateFromJsonFile() {

	vector<RsbParameters> newRsbData;
	vector<RcbParameters> newRcbData;
	SniParameters newSniPars;
	unsigned int axleCntUpperEnd, axleCntLowerEnd;

	//parse configuration file and validate new parameters
	try {
		parseJsonFile(newRsbData, newRcbData, newSniPars);
		validateParameters(newRsbData, newRcbData, newSniPars, &axleCntUpperEnd, &axleCntLowerEnd);
	}
	catch (...) {
		sysLog << "Error: cannot update configuration parameters from JSON file";
		sysLog.writeAsError();
		throw;
	}

	//check if there are any changes in new configuration,
	//log event and update changes if necessary
	if ((newRsbData != rsbData) || (newRcbData != rcbData) || (newSniPars != sniPars)) {

		rsbData = newRsbData;
		rcbData = newRcbData;
		sniPars = newSniPars;
		numRsbSensAtUpperEnd = axleCntUpperEnd;
		numRsbSensAtLowerEnd = axleCntLowerEnd;
		setRefArrays();

		sysLog << "Info: new configuration parameters detected and successfully updated";
		sysLog.writeAsInfo();
	}

}

const SniParameters& ConfigManager::getSniPars() const {
	return sniPars;
}

const unsigned int ConfigManager::getNumRsbs() const {
	return rsbData.size();
}

const unsigned int ConfigManager::getNumRcbs() const {
	return rcbData.size();
}

const unsigned int ConfigManager::getNumRcbPairs() const {
	return rcbData.size()/2;
}

RsbParameters& ConfigManager::getRsb(unsigned int rsbLinearIdx) {
	if (rsbLinearIdx >= getNumRsbs()) {
		throw runtime_error("Error: RSB index out of bounds");
	}
	return rsbData[rsbLinearIdx];
}

RcbParameters& ConfigManager::getRcb(unsigned int rcbLinearIdx) {
	if (rcbLinearIdx >= getNumRcbs()) {
		throw runtime_error("Error: RCB index out of bounds");
	}
	return rcbData[rcbLinearIdx];
}

RcbParameters& ConfigManager::getRcb(unsigned int rcbPairIdx, unsigned int rcbRailIdx) {
	if (rcbRailIdx > 1) {
		throw runtime_error("Error: RCB index out of bounds");
	}
	return getRcb(2*rcbPairIdx + rcbRailIdx);
}

RcbParameters& ConfigManager::getLeftRcb(unsigned int rcbPairIdx) {
	return getRcb(rcbPairIdx, 0);
}

RcbParameters& ConfigManager::getRigthRcb(unsigned int rcbPairIdx) {
	return getRcb(rcbPairIdx, 1);
}

const vector<SensorParsWithMeasData*>& ConfigManager::getRcbSensorPois() const {
	return rcbSensorPois;
}

const vector<SensorParsWithMeasData*>& ConfigManager::getRsbSensorPois() const {
	return rsbSensorPois;
}

const vector<SensorParsWithMeasData*> ConfigManager::getReliableRsbSensorPois() {

	vector<SensorParsWithMeasData*> reliableRsbSensorPois;
	unsigned int rsbIdx;

	//create vector for reliable RSB modules
	for (rsbIdx = 0; rsbIdx < rsbData.size(); rsbIdx++) {
		if (rsbData[rsbIdx].isFailureState() == false) {
			reliableRsbSensorPois.push_back(&(rsbData[rsbIdx]));
		}
	}

	return reliableRsbSensorPois;
}

bool ConfigManager::isRsbAliveAtBothEnds() const {

	unsigned int rsbIdx;
	vector<bool> sensPos;

	//collect sensor position data from each alive RSB
	for (rsbIdx = 0; rsbIdx < getNumRsbs(); rsbIdx++) {
		if ((rsbData[rsbIdx].isAlive() == true) && (rsbData[rsbIdx].isFailureState() == false)) {
			sensPos.insert(sensPos.end(), rsbData[rsbIdx].isSensAtUpperEnd().begin(), rsbData[rsbIdx].isSensAtUpperEnd().end());
		}
	}

	//count true and false elements in vector, return appropriately
	if ((count(sensPos.begin(), sensPos.end(), true) < 1) ||
		(count(sensPos.begin(), sensPos.end(), false) < 1)) {
		return false;
	}
	else {
		return true;
	}
}

void ConfigManager::resetAliveFlags() {

	unsigned int sensIdx;

	//set alive flag of each sensor true, reset status bits
	for (sensIdx = 0; sensIdx < getNumRsbs(); sensIdx++) {
		getRsb(sensIdx).setAlive(true);
	}
	for (sensIdx = 0; sensIdx < getNumRcbs(); sensIdx++) {
		getRcb(sensIdx).setAlive(true);
	}
}

void ConfigManager::resetStatusBits() {

	unsigned int sensIdx;

	//set alive flag of each sensor true, reset status bits
	for (sensIdx = 0; sensIdx < getNumRsbs(); sensIdx++) {
		getRsb(sensIdx).resetStatusBits();
	}
	for (sensIdx = 0; sensIdx < getNumRcbs(); sensIdx++) {
		getRcb(sensIdx).resetStatusBits();
	}
}

void ConfigManager::resetRsbFailureStates() {

	unsigned int sensIdx;

	//resetting the failure state for each RSB
	for (sensIdx = 0; sensIdx < getNumRsbs(); sensIdx++) {
		getRsb(sensIdx).resetFailureState();
	}
}

void ConfigManager::setRsbFailueState(char addr) {

	unsigned int sensIdx;

	//set failure state for the RSB with the given address
	for (sensIdx = 0; sensIdx < getNumRsbs(); sensIdx++) {
		if (getRsb(sensIdx).getAddress() == addr) {
			getRsb(sensIdx).setFailureState();
			getRsb(sensIdx).addStatusBitMask(SensorParameters::rsbErrorFailureStateMask);
		}
	}
}

unsigned int ConfigManager::getNumRsbSensAtLowerEnd() const {
	return numRsbSensAtLowerEnd;
}

unsigned int ConfigManager::getNumRsbSensAtUpperEnd() const {
	return numRsbSensAtUpperEnd;
}

void ConfigManager::parseJsonFile(vector<RsbParameters>& rsbDataOut, vector<RcbParameters>& rcbDataOut, SniParameters& sniParsOut) const {

	ifstream jsonFile;
	stringstream fileStream;
	unsigned int objIdx;

	//clear input vectors
	rsbDataOut.clear();
	rcbDataOut.clear();

	//try opening json file, log and return if unsuccessful
	jsonFile.open(jsonFileName.c_str());
	if ((jsonFile.good() == false) || (jsonFile.is_open() == false)) {
		sysLog << "Error: configuration file " << jsonFileName << " could not be opened";
		sysLog.writeAsError();
		throw runtime_error("Configuration file open failed");
	}

	//process json file, log possible errors and return
	try {

		//convert file content to string, parse as json file, check if successful
		fileStream << jsonFile.rdbuf();
		json::Value jsonVal = json::Deserialize(fileStream.str());
		if (jsonVal.GetType() == json::NULLVal) {
			throw runtime_error("JSON syntax error");
		}

		//convert general value to json object
		json::Object jsonConfData = jsonVal.ToObject();

		//get RSB data
		json::Array rsbArray = getArrayJsonField(jsonConfData, "RSBs");
		for (objIdx = 0; objIdx < rsbArray.size(); objIdx++) {
			rsbDataOut.push_back(RsbParameters(rsbArray[objIdx]));
		}

		//get RCB data
		json::Array rcbArray = getArrayJsonField(jsonConfData, "RCBs");
		for (objIdx = 0; objIdx < rcbArray.size(); objIdx++) {
			rcbDataOut.push_back(RcbParameters(rcbArray[objIdx]));
		}

		//get general parameters
		sniParsOut = SniParameters(jsonConfData);
	}
	catch (exception& currExc) {
		sysLog << "Error: invalid configuration file: " << currExc.what();
		sysLog.writeAsError();
		throw("JSON file error");
	}
}

void ConfigManager::validateParameters(vector<RsbParameters>& rsbDataInOut, vector<RcbParameters>& rcbDataInOut, const SniParameters& sniParsIn, unsigned int* axleCntUpperEnd, unsigned int* axleCntLowerEnd) const {

	//use a single catch block in the end for logging any problem
	try {

		//first of all, check that input vectors are not empty
		if ((rsbDataInOut.empty() == true) || (rcbDataInOut.empty() == true)) {
			throw runtime_error("Error: no RCB/RSB entries found");
		}

		//create unified vector for RSBs and RCBs, and ensure that address fields are unique
		vector<SensorParameters> allSensPars;
		allSensPars.insert(allSensPars.end(), rsbDataInOut.begin(), rsbDataInOut.end());
		allSensPars.insert(allSensPars.end(), rcbDataInOut.begin(), rcbDataInOut.end());

		sort(allSensPars.begin(), allSensPars.end(), IsLessSensParAddr());
		if (unique(allSensPars.begin(), allSensPars.end(), IsEqualSensParAddr()) != allSensPars.end()) {
			throw runtime_error("Error: RSB/RCB address fields are not unique");
		}

		//sort RCB modules according to physical location, check RCB physical arrangement
		//	- there must be an even number of RCB sensors
		//	- there is a minimal allowed number of RCB sensors
		//	- positions must be unique
		//	- sensor pairs must have the same position_mm (if vector is sorted and positions
		//	  are unique, the first one will be on left rail and the second on right one,
		//	  so it is not necessary to check this)
		sort(rcbDataInOut.begin(), rcbDataInOut.end(), IsLessRcbParPos());

		unsigned int numRcbs = rcbDataInOut.size();
		if ((numRcbs % 2 != 0) || (numRcbs < ConfigLimits::minNumRcb)) {
			throw runtime_error("Error: wrong number of RCB modules in system");
		}

		if (unique(rcbDataInOut.begin(), rcbDataInOut.end(), IsEqualRcbParPos()) != rcbDataInOut.end()) {
			throw runtime_error("Error: RCB positions are not unique");
		}

		unsigned int sensPairIdx;
		for (sensPairIdx = 0; sensPairIdx < numRcbs; sensPairIdx += 2) {
			if (rcbDataInOut[sensPairIdx].getPosition_mm() != rcbDataInOut[sensPairIdx+1].getPosition_mm()) {
				throw runtime_error("Error: a pair of RCB modules are not opposite to each other");
			}
		}

		//for each sensor of every RSB module, determine if the sensor is at the upper
		//or lower end of the system - checking if each RSB sensor is"outside RCB area"
		//boolean vector will be set with appropriate method, will throw exception
		//in case of invalid RSB position
		//also calculate the number of total upper/lower end sensors and check if there
		//is at least the specified number of RSB sensors at both end of the system
		unsigned int rsbIdx;
		vector<bool> allSensAtUpperEnd;
		for (rsbIdx = 0; rsbIdx < rsbDataInOut.size(); rsbIdx++) {
			rsbDataInOut[rsbIdx].setSensAtUpperEnd(rcbDataInOut[0].getPosition_mm(), rcbDataInOut[numRcbs-1].getPosition_mm());
			allSensAtUpperEnd.insert(allSensAtUpperEnd.end(),
					rsbDataInOut[rsbIdx].isSensAtUpperEnd().begin(), rsbDataInOut[rsbIdx].isSensAtUpperEnd().end());
		}

		*axleCntUpperEnd = (unsigned int) count(allSensAtUpperEnd.begin(), allSensAtUpperEnd.end(), true);
		*axleCntLowerEnd = (unsigned int) count(allSensAtUpperEnd.begin(), allSensAtUpperEnd.end(), false);

		if ((*axleCntUpperEnd < ConfigLimits::minNumRsbAtBorder) ||
			(*axleCntLowerEnd < ConfigLimits::minNumRsbAtBorder)) {
			throw runtime_error("Error: not enough RSB sensors at one end of the system");
		}

		//check if the number of sensors for each RSB is below the specified maximal value
		//and it is at least one
		for (rsbIdx = 0; rsbIdx < rsbDataInOut.size(); rsbIdx++) {
			assertOutsideRange<unsigned int>(rsbDataInOut[rsbIdx].getNumSens(), 1, ConfigLimits::maxNumRcbSens);
		}

		//validate general parameters - exception will be thrown if outside range
		assertOutsideRange<float>(sniParsIn.getGlobalScale(), ConfigLimits::minGlobalScale, ConfigLimits::maxGlobalScale);
		assertOutsideSet<unsigned int>(sniParsIn.getNumSamples(),  ConfigLimits::validNumSamples, ConfigLimits::numValidNumSamples);

	}
	catch (exception& currExc) {
		sysLog << currExc.what();
		sysLog.writeAsError();
		throw;
	}
}

void ConfigManager::setRefArrays() {

	unsigned int sensIdx;

	rsbSensorPois.clear();
	for (sensIdx = 0; sensIdx < rsbData.size(); sensIdx++) {
		rsbSensorPois.push_back(&(rsbData[sensIdx]));
	}
	rcbSensorPois.clear();
	for (sensIdx = 0; sensIdx < rcbData.size(); sensIdx++) {
		rcbSensorPois.push_back(&(rcbData[sensIdx]));
	}
}

void ConfigManager::addMeasFile(const string& offlineMeasFileName) {

	ifstream measFile;
	stringstream fileStream;
	json::Object measObj;

	//check if file extension is json
	if (offlineMeasFileName.rfind(".json") != offlineMeasFileName.length()-5) {
		throw runtime_error("Unknown measurement file format");
	}

	//process json file
	measFile.open(offlineMeasFileName.c_str());
	if ((measFile.good() == false) || (measFile.is_open() == false)) {
		sysLog << "Error: measurement file " << offlineMeasFileName << " could not be opened";
		sysLog.writeAsError();
		throw runtime_error("Measurement file open failed");
	}

	//convert file content to string, parse as json file, check if successful
	fileStream << measFile.rdbuf();
	json::Value jsonVal = json::Deserialize(fileStream.str());
	if (jsonVal.GetType() == json::NULLVal) {
		throw runtime_error("JSON syntax error");
	}

	//convert general value to json object
	measObj = jsonVal.ToObject();

	//get and process RSB data
	json::Array rsbArray = getArrayJsonField(measObj, "RSBs");
	addMeasSensors(rsbArray, rsbSensorPois);

	//get and process RCB data
	json::Array rcbArray = getArrayJsonField(measObj, "RCBs");
	addMeasSensors(rcbArray, rcbSensorPois);
}

void ConfigManager::addMeasSensors(const json::Array& sensorData, const vector<SensorParsWithMeasData*>& sensorPoiArray) {

	unsigned int sensIdx, objIdx, measIdx;

	//set all sensors as unalive
	for (sensIdx = 0; sensIdx < sensorPoiArray.size(); sensIdx++) {
		sensorPoiArray[sensIdx]->setAlive(false);
	}

	for (objIdx = 0; objIdx < sensorData.size(); objIdx++) {

		//current sensor data object
		json::Object currSensObj = sensorData[objIdx];

		//get address, number of entries and entry array
		int currAddr;
		stringstream convStr;
		string addrStr = getStringJsonField(currSensObj, "Add");
		convStr << hex << addrStr.substr(2);
		convStr >> currAddr;
		int numMeas = getIntJsonField(currSensObj, "NumE");
		json::Array measArr = getArrayJsonField(currSensObj, "E");

		//search for sensor based on address
		SensorParsWithMeasData* currSensPoi = NULL;
		for (sensIdx = 0; sensIdx < sensorPoiArray.size(); sensIdx++) {
			if (sensorPoiArray[sensIdx]->getAddress() == (int (currAddr))) {
				currSensPoi = sensorPoiArray[sensIdx];
			}
		}

		//check if sensor exists
		if (currSensPoi == NULL) {
			throw runtime_error(string("Error: sensor ").append(addrStr).append(string(" does not exist")));
		}

		//add entries
		if (numMeas > 0) {
			currSensPoi->setAlive(true);
			currSensPoi->setNumMeasDummy(numMeas);
			for (measIdx = 0; measIdx < measArr.size(); measIdx++) {
				json::Object currMeas = measArr[measIdx];
				int axleId = getIntJsonField(currMeas, "Ax");
				if (axleId > numMeas) {
					throw runtime_error("Invaild axle ID found");
				}
				currSensPoi->setMeasFromJson(currMeas, axleId);
			}
		}
	}
}

void ConfigManager::addMeasString(const string& offlineMeasData) {

	json::Object measObj;

	json::Value jsonVal = json::Deserialize(offlineMeasData);
	if (jsonVal.GetType() == json::NULLVal) {
		throw runtime_error("JSON syntax error");
	}

	//convert general value to json object
	measObj = jsonVal.ToObject();

	//get and process RSB data
	json::Array rsbArray = getArrayJsonField(measObj, "RSBs");
	addMeasSensors(rsbArray, rsbSensorPois);

	//get and process RCB data
	json::Array rcbArray = getArrayJsonField(measObj, "RCBs");
	addMeasSensors(rcbArray, rcbSensorPois);
}

} /* namespace sni */


