/*
 * evopro Innovation Kft.
 *
 * sni
 *
 * File: ConfigManager.h
 * <Description>
 * Created on: Aug 13, 2014
 *
 * Author: pechan
 * (C) 2014. evopro Innovation Kft.
 *
 */

#ifndef ERDM_PC_CONFIGMANAGER_H_
#define ERDM_PC_CONFIGMANAGER_H_

#include <string>
#include <vector>

#include "Logger.h"
#include "json.h"
#include "SerialProtocol.h"

using namespace sniutils;
using namespace sniprotocol;
using namespace std;

namespace sni {

//class for storing constants
class ConfigLimits {
public:
	//minimal allowed number of RCB modules
	static const unsigned int minNumRcb;
	//minimal allowed number of RSB sensors at both end of the system
	static const unsigned int minNumRsbAtBorder;
	//maximal allowed number of sensors per RSB modules
	static const unsigned int maxNumRcbSens;
	//minimal and maximal allowed value for global scale factor
	static const float minGlobalScale;
	static const float maxGlobalScale;
	//allowed values for numSamples
	static const unsigned int numValidNumSamples;
	static const unsigned int validNumSamples[];
private:
	ConfigLimits();
};

//class for storing general sensor parameters, which are extracted from a JSON object
//	- constructor throws exception in case of error conditions
//	- getters are available for accessing parameters
//	- alive flag can be used for storing sensor status
//	- comparison operators only compare fields read from config file,
//	  and can be used for determining if the configuration changed
class SensorParameters {
public:
	static const uint16_t comErrorResetMask;
	static const uint16_t comErrorEchoMask;
	static const uint16_t comErrorAutoZeroMask;
	static const uint16_t statErrorAutoZeroMask;
	static const uint16_t comErrorArmMask;
	static const uint16_t statErrorArmMask;
	static const uint16_t comErrorCheckStateMask;
	static const uint16_t statErrorNotArmedMask;
	static const uint16_t comErrorWhileMeasMask;
	static const uint16_t comErrorDisarmMask;
	static const uint16_t statErrorDisarmMask;
	static const uint16_t comErrorGetAxcntMask;
	static const uint16_t comErrorGetMeasMask;
	static const uint16_t rsbErrorFailureStateMask;

	SensorParameters(const json::Object& jsonObj);
	char getAddress() const;
	uint32_t getId() const;
	void setId(uint32_t newId);
	bool isIdValid() const;
	bool isTempSensor() const;
	bool operator==(const SensorParameters& rhs) const;
	bool operator!=(const SensorParameters& rhs) const;
	bool isAlive() const;
	void setAlive(bool alive);
	void resetStatusBits();
	uint16_t getStatusBits() const;
	void addStatusBitMask(uint16_t statMask);

private:
	uint32_t ID;
	uint16_t statusBits;
	char address;
	bool tempSensor;
	bool alive;
	bool idValid;
};

//functor class for comparing SensorParameters objects based on their address
class IsLessSensParAddr {
public:
	bool operator() (const SensorParameters& first, const SensorParameters& second) {

		return (first.getAddress() < second.getAddress());
	}
};

//functor class for comparing SensorParameters objects based on their address
class IsEqualSensParAddr {
public:
	bool operator() (const SensorParameters& first, const SensorParameters& second) {

		return (first.getAddress() == second.getAddress());
	}
};

//class for adding polymorphic functions for accessing measurement data
//or setting them from measurement file
class SensorParsWithMeasData: public SensorParameters {
public:
	SensorParsWithMeasData(const json::Object& jsonObj) : SensorParameters(jsonObj) {}
	virtual ~SensorParsWithMeasData() {}
	virtual unsigned int getNumMeas() const = 0;
	virtual MeasLdataAnswData& getSingleMeas(unsigned int measIdx) = 0;
	virtual void setNumMeasDummy(unsigned int numMeas) = 0;
	virtual void setMeasFromJson(const json::Object& measObj, unsigned int measIdx) = 0;
};

//class for storing RSB parameters, see description at SensorParameters
//	- setSensAtUpperEnd sets the boolean vector sensAtUpperEnd which stores
//	  a bool for each sensor position in vector sensPositions_mm a bool indicating
//	  if the given sensor of the RSB is at the lower end (false) or upper end (true)
//	  of the sensor network (lower end corresponds to lower mm position)
//	  calculation is based on the position of the lowest and highest RCB
//	  throws exception if any RSB sensor is within the specified interval
//	- comparison operators only compare fields read from config file,
//	  and can be used for determining if the configuration changed
class RsbParameters: public SensorParsWithMeasData {
public:
	RsbParameters(const json::Object& jsonObj);
	const vector<int>& getSensPositions_mm() const;
	void setSensAtUpperEnd(int lowestRcbPos_mm, int highestRcbPos_mm);
	const vector<bool>& isSensAtUpperEnd() const;
	unsigned int getNumSens() const;
	bool isSensAtUpperEnd(unsigned int sensIdx) const;
	bool operator==(const RsbParameters& rhs) const;
	bool operator!=(const RsbParameters& rhs) const;
	vector<RsbMeasLdataAnswData>& getMeasData();
	virtual unsigned int getNumMeas() const;
	virtual MeasLdataAnswData& getSingleMeas(unsigned int measIdx);
	RsbMeasLdataAnswData& getSingleMeasFull(unsigned int measIdx);
	virtual void setNumMeasDummy(unsigned int numMeas);
	virtual void setMeasFromJson(const json::Object& measObj, unsigned int measIdx);
	void setFailureState();
	void resetFailureState();
	bool isFailureState() const;

private:
	vector<int> sensPositions_mm;
	vector<bool> sensAtUpperEnd;
	vector<RsbMeasLdataAnswData> measData;
	bool failureState;
};

//class for storing RCB parameters, see description at SensorParameters
//	- comparison operators only compare fields read from config file,
//	  and can be used for determining if the configuration changedl
class RcbParameters: public SensorParsWithMeasData {
public:
	RcbParameters(const json::Object& jsonObj);
	bool isOnLeftRail() const;
	int getPosition_mm() const;
	float getScaleOffset(bool dirFromLowerToUpper) const;
	float getScaleSpeed(bool dirFromLowerToUpper) const;
	float getScaleTemp(bool dirFromLowerToUpper) const;
	bool operator==(const RcbParameters& rhs) const;
	bool operator!=(const RcbParameters& rhs) const;
	vector<RcbMeasLdataAnswData>& getMeasData();
	virtual unsigned int getNumMeas() const;
	virtual MeasLdataAnswData& getSingleMeas(unsigned int measIdx);
	RcbMeasLdataAnswData& getSingleMeasFull(unsigned int measIdx);
	virtual void setNumMeasDummy(unsigned int numMeas);
	virtual void setMeasFromJson(const json::Object& measObj, unsigned int measIdx);

private:
	bool onLeftRail;
	int position_mm;
	float scaleOffset[2];
	float scaleTemp[2];
	float scaleSpeed[2];
	vector<RcbMeasLdataAnswData> measData;
};

//functor class for comparing RcbParameters based on their position
//an RcbParameters object is considered "less" then another one ifs
//position_mm is less, or if it is equal but it is on the left rail
//and the other one is on the right rail
class IsLessRcbParPos {
public:
	bool operator() (const RcbParameters& first, const RcbParameters& second) {

		return ((first.getPosition_mm() < second.getPosition_mm()) ||
		   ((first.getPosition_mm() == second.getPosition_mm()) && (first.isOnLeftRail())));
	}
};

//function class for comparing RcbParameters based on their position
class IsEqualRcbParPos {
public:
	bool operator() (const RcbParameters& first, const RcbParameters& second) {

		return ((first.getPosition_mm() == second.getPosition_mm()) && (first.isOnLeftRail() == second.isOnLeftRail()));
	}
};

//class for storing general parameters
//	- constructor throws exception in case of error conditions
//	- getters are available for accessing parameters
class SniParameters {
public:
	SniParameters();
	SniParameters(const json::Object& jsonObj);
	float getGlobalScale() const;
	unsigned int getNumSamples() const;
	bool operator==(const SniParameters& rhs) const;
	bool operator!=(const SniParameters& rhs) const;

private:
	float globalScale;
	unsigned int numSamples;
	//TODO: add parameter handling bridge signedness - but actually, this is a per-sensor parameter...
	//TODO: for every configurable parameter add sanity check at ConfigManager::validateParameters
};

//class for storing and handling sni and sensor network configuration parameters extracted from a JSON file,
//and also the per-sensor measurement data
//the class logs any error conditions to system log
//	- constructor may throw exception in case of error
//	  if the third parameter is provided, the provided measurement file will be processed and measData field
//	  of rsbData and rcbData will be filled, as if the measurement results had been extracted from the sensors
//	  throws exception in case of errors
//	- updateFromJsonFile() parses the file again and updates parameters in case of change
//	  throws exception in case of any error conditions
//	- per-index getters may throw exception if index is out of bounds, RCB/RSB indexing:
//	  linearIdx: 0..(number_of_sensors-1)
//	  pairIdx: 0..(number_of_sensor_pairs-1)
//	  railIdx: 0 for left, 1 for right rail
//	- isRsbAliveAtBothEnds() returns true, if there is at least one alive RSB sensor at both ends of the system
class ConfigManager {
public:
	ConfigManager(Logger& sysLogInit, const string& jsonFileNameInit, string offlineMeasFileName = string(""));
	~ConfigManager();
	void updateFromJsonFile();
	const SniParameters& getSniPars() const;
	const unsigned int getNumRsbs() const;
	const unsigned int getNumRcbs() const;
	const unsigned int getNumRcbPairs() const;
	RsbParameters& getRsb(unsigned int rsbLinearIdx);
	RcbParameters& getRcb(unsigned int rcbLinearIdx);
	RcbParameters& getRcb(unsigned int rcbPairIdx, unsigned int rcbRailIdx);
	RcbParameters& getLeftRcb(unsigned int rcbPairIdx);
	RcbParameters& getRigthRcb(unsigned int rcbPairIdx);
	const vector<SensorParsWithMeasData*>& getRcbSensorPois() const;
	const vector<SensorParsWithMeasData*>& getRsbSensorPois() const;
	const vector<SensorParsWithMeasData*> getReliableRsbSensorPois();
	bool isRsbAliveAtBothEnds() const;
	void resetAliveFlags();
	void resetStatusBits();
	void resetRsbFailureStates();
	void setRsbFailueState(char addr);
	unsigned int getNumRsbSensAtLowerEnd() const;
	unsigned int getNumRsbSensAtUpperEnd() const;

	void addMeasString(const string& offlineMeasData); // Based on ConfigManager::addMeasFile

private:
	Logger& sysLog;
	string jsonFileName;
	vector<RsbParameters> rsbData;
	vector<RcbParameters> rcbData;
	vector<SensorParsWithMeasData*> rsbSensorPois;
	vector<SensorParsWithMeasData*> rcbSensorPois;
	SniParameters sniPars;
	unsigned int numRsbSensAtUpperEnd;
	unsigned int numRsbSensAtLowerEnd;
	void parseJsonFile(vector<RsbParameters>& rsbDataOut, vector<RcbParameters>& rcbDataOut, SniParameters& sniParsOut) const;
	void validateParameters(vector<RsbParameters>& rsbDataOut, vector<RcbParameters>& rcbDataOut, const SniParameters& sniParsIn, unsigned int* axleCntUpperEnd, unsigned int* axleCntLowerEnd) const;
	void setRefArrays();
	void addMeasFile(const string& offlineMeasFileName);
	void addMeasSensors(const json::Array& sensorData, const vector<SensorParsWithMeasData*>& sensorPoiArray);

	template <class T>
	void assertOutsideRange(T val, T minVal, T maxVal) const {
		if ((val < minVal) || (val > maxVal)) throw runtime_error("Error: parameter out of range");
	}

	template <class T>
	void assertOutsideSet(T val, const T valSet[], unsigned int numVals) const {
		if (find(valSet, valSet+numVals, val) == valSet+numVals) throw runtime_error("Error: parameter out of range");
	}
};

} /* namespace sni */
#endif /* ERDM_PC_CONFIGMANAGER_H_ */
