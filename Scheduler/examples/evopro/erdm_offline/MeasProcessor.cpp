/*
 * MeasProcessor.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: pechan
 */

#include <vector>
#include <algorithm>
#include <numeric>
#include <string>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iomanip>

#include "MeasProcessor.h"
#include "Utils.h"
#include "Config.h"

namespace sni {

//algorithm parameters
const bool AlgPars::discardExtremums = true;
const unsigned int AlgPars::discardIfAtLeast = 5;
const float AlgPars::lsRhoLowerBound = 0.06f;
const unsigned int AlgPars::lsMaxNumIters = 300;
const unsigned int AlgPars::lsMaxConsSucc = 4;
const unsigned int AlgPars::lsMaxConsFail = 4;
const float AlgPars::lsExpansionFactor = 2.0f;
const float AlgPars::lsContractionFactor = 0.5f;
const float AlgPars::lsRandIntervalScale = 100.0f;
const float AlgPars::lsRandIntervalPad = 10.0f;
const float AlgPars::lsPadMin = -80.0f;
const float AlgPars::lsPadMax = 500.0f;
const float AlgPars::lsScaleMin = 0.0f;
const float AlgPars::lsScaleMax = 65000.0f;
const unsigned int AlgPars::pulseLengthLowLim = 50;
const unsigned int AlgPars::rsbPulseLengthLowLim = 50;
const float AlgPars::pulseLengthTolerance = 0.25f;
const float AlgPars::centMassLowLim = 43.75f;
const float AlgPars::centMassHighLim = 56.25f;
const float AlgPars::invldELim = 100.0f;
const float AlgPars::invldSmoothLim = 1000.0f;
const float AlgPars::dmgSmoothLim = 20.0f;
const float AlgPars::limitTsErr = 10.0f;
const float AlgPars::limitTsErrDiff = 5.0f;
const float AlgPars::limitFitSensRate = 2.0f/3.0f*100.0f;
const float AlgPars::maxTsDiffChange = 2.0f;
const float AlgPars::wdPadHighLim = 150.0f;
const unsigned int AlgPars::wdMaxHitFlatWidthSamp32 = 6;
const float AlgPars::speedMax_mps = 200.0f/3.6f;
const float AlgPars::tailIgnoreDuringFit = 6.25f;

//minimal distance between consecutive axles is 0.7m (ROLA),
//maximal train speed is 160km/h -> dt = 0.7/(160/3.6) = 0.0158s
const unsigned int AlgPars::rsbPulseDistLowLim = 150;

//////////////////////////////////////////////////////////
//		function definitions for class MeasEntity
//////////////////////////////////////////////////////////
MeasEntity::MeasEntity(RcbMeasLdataAnswData& measEntryInit) :
	measEntry(&measEntryInit) {

	stringstream convStr;

	//check if entry is valid and pulse length is OK
	if (measEntry->isDummy() == true) {
		throw runtime_error("Entry is missing");
	}
	if (measEntry->isValid() == false) {
		convStr << measEntry->getStatus();
		throw runtime_error(string("Entry is invalid, stat field is: ").append(convStr.str()));
	}
	if (measEntry->getPulseLength100us() < AlgPars::pulseLengthLowLim) {
		convStr << measEntry->getPulseLength100us();
		throw PulseLengthExc(string("Entry pulse length is ").append(convStr.str()));
	}

	//get initial axle ID and set assign and usable flag
	axleIdx = measEntry->getAxleId();
	isAssigned = false;
	isUsable = false;
	isUsableForWd = false;
}

vector<int16_t> MeasEntity::calcConvSamples() const {

	vector<int16_t> convSamples;

	convSamples.resize(measEntry->getSamples().size());
	transform(measEntry->getSamples().begin(), measEntry->getSamples().end(), convSamples.begin(), ConvSamples(measEntry->getOffset()));

	return convSamples;
}

vector<int> MeasEntity::calcFittedCurve() const {

	//create vector for limit values (unused in this case)
	ArithmeticVector dummyLimits;
	dummyLimits.resize(3, 0.0f);

	//create parameter vector based on fitted values
	ArithmeticVector initParVec;
	initParVec.push_back(curveParams.fitScale);
	initParVec.push_back(curveParams.fitPadLeft);
	initParVec.push_back(curveParams.fitPadRight);

	//calcualte fitted curve samples
	CurveFittingError fitError(calcConvSamples(), dummyLimits, dummyLimits);
	fitError.getScore(initParVec);

	return fitError.getTransformedCurve();
}

//////////////////////////////////////////////////////////
//		function definitions for class MeasProcessor
//////////////////////////////////////////////////////////

MeasProcessor::MeasProcessor(ConfigManager& confMgrInit, Logger& sysLogInit) :
	confMgr(confMgrInit),
	sysLog(sysLogInit) {
}

void MeasProcessor::doProcessing() {

	bool axleSpeedOk;

	//create measurement entity objects, init status flags
	filterRsbEnts();
	initMeasEnts(true);
	alOkWoutTsRest = false;
	alOkWithTsRest = false;
	alUsingDef = false;

	//perform timestamp alignment
	try {

		//TODO: if necessary, each upper-lower reference timestamp vector pairs could be tested
		//until one of them satisfies reliability requirement
		//capture reference timestamps
		initRefTs();

		//if the number of reference timestamps initially equal,
		//try alignment and check alignment against hard condition
		if (upperRefTs100us.size() == lowerRefTs100us.size()) {
			calcAxleTs();
			alignMeasEnts();
			alOkWoutTsRest = evalAlignment(true);
		}

		//if hard condition was not fulfilled or numbers of timestamps
		//do not equal, reinit measurement entities (possibly containing
		//invalid alignment data), try restoring timestamps and check alignment
		//against soft condition
		if (alOkWoutTsRest == false) {
			initMeasEnts();
			restoreTs();
			axleSpeedOk = calcAxleTs();
			if (axleSpeedOk == false) {
				throw runtime_error("Invalid axle speed");
			}
			alignMeasEnts();
			alOkWithTsRest = evalAlignment(false);
		}

		//even if alignment is unsuccessful, it will be used, but log event
		if ((alOkWithTsRest == false) && (alOkWoutTsRest == false)) {
			sysLog << "Error restoring timestamps, measurement will be marked unreliable";
			sysLog.writeAsError();
		}

		//remove measurement entities with high timestamp error from alignment,
		//these will not be used during further processing steps
		disassignErrEnts();
	}
	catch (exception& currExc) {

		//reinitialize measurement entity vectors (possibliy containing invalid
		//alignment data), set default alignment and status flag, also log event
		sysLog << "Error: using default axle alignment, measurement will be marked unreliable" << " (" << currExc.what() << ")";
		sysLog.writeAsError();

		initMeasEnts();
		assignDefault();
		alUsingDef = true;
	}

	//perform curve fitting and curve evaluation steps for assigned measurement entities
	processCurves();

	//calculate temperature values
	calcTemps();

	//determine load value for each measurement entity
	scaleHeights();

	//calculate result values for axles
	calcAxleResults();

	//calculate sensor status values
	calcSensorStats();
}

void MeasProcessor::getSerializedResult(uint64_t timestamp, bool axleNumOk, vector<uint8_t>& serRes) {

	unsigned int axleIdx, sensIdx;

	//clear vector, reserve initial size
	serRes.clear();
	serRes.reserve(22 + 15*axleData.size() + 10*(confMgr.getNumRcbs()+confMgr.getNumRsbs()));

	//add timestamp field
	Utils::addSerialized(timestamp, serRes);

	//add speed fields
	uint16_t speedFirst = 0;
	uint16_t speedLast = 0;
	if (axleData.empty() == false) {
		speedFirst = Utils::float2ushort(axleData[0].speed_mps*3.6f);
		speedLast = Utils::float2ushort(axleData[axleData.size()-1].speed_mps*3.6f);
	}
	Utils::addSerialized(speedFirst, serRes);
	Utils::addSerialized(speedLast, serRes);

	//assemble and add status field
	uint32_t measStatus = 0;
	if (directionLow2Up == true) {
		measStatus |= 0x00000001;
	}
	if ((alUsingDef == false) && ((alOkWoutTsRest == true) || (alOkWithTsRest == true))) {
		measStatus |= 0x00000002;
	}
	if (axleNumOk == false) {
		measStatus |= 0x00000004;
	}
	if (measAvgTempDefUsed == true) {
		measStatus |= 0x00000008;
	}
	Utils::addSerialized(measStatus, serRes);

	//add temperature field
	Utils::addSerialized(measAvgTemp, serRes);

	//add number of axles
	uint16_t numOfAxles = uint16_t (axleData.size());
	Utils::addSerialized(numOfAxles, serRes);

	//for each axle, add fields
	for (axleIdx = 0; axleIdx < numOfAxles; axleIdx++) {

		//add first the left, then the right side loads of the train, when left and right is
		//relative to train direction (if the train passed from lower to upper sensor positions,
		//this is the same 'left' and 'right' as the physical sensor location, otherwise they
		//must be switched
		if (directionLow2Up == true) {

			//add left load for each axle
			Utils::addSerialized(Utils::float2ushort(axleData[axleIdx].loadLeft_kg), serRes);

			//add right load for each axle
			Utils::addSerialized(Utils::float2ushort(axleData[axleIdx].loadRight_kg), serRes);
		}
		else {

			//add right load for each axle
			Utils::addSerialized(Utils::float2ushort(axleData[axleIdx].loadRight_kg), serRes);

			//add left load for each axle
			Utils::addSerialized(Utils::float2ushort(axleData[axleIdx].loadLeft_kg), serRes);
		}

		//add axle load for each axle
		Utils::addSerialized(Utils::float2ushort(axleData[axleIdx].loadAx_kg), serRes);

		//add standard deviation for each axle
		Utils::addSerialized(Utils::float2ushort(axleData[axleIdx].stdDevLoadAx_kg), serRes);

		//add number of sensor pairs for each axle
		serRes.push_back(axleData[axleIdx].numUsedSensPairs);

		//add worstE value for each axle
		serRes.push_back(axleData[axleIdx].worstE);

		//add worst_wdE value for each axle
		serRes.push_back(axleData[axleIdx].worstWdE);

		//add wheel diagnostics value for each axle
		Utils::addSerialized(axleData[axleIdx].wheelDiag, serRes);

		//add axle position value
		Utils::addSerialized(axleData[axleIdx].pos_mm, serRes);
	}

	//add number of sensors
	uint16_t numOfSensors = uint16_t (confMgr.getNumRsbs() + confMgr.getNumRcbs());
	Utils::addSerialized(numOfSensors, serRes);

	//for each RSB add fields
	for (sensIdx = 0; sensIdx < confMgr.getNumRsbs(); sensIdx++) {
		Utils::addSerialized(uint32_t (confMgr.getRsb(sensIdx).getId()), serRes);
		serRes.push_back(uint8_t (confMgr.getRsb(sensIdx).getAddress()));
		Utils::addSerialized(rsbStat[sensIdx], serRes);
		uint32_t stat = 0x00;
		if (confMgr.getRsb(sensIdx).isAlive() == true) {
			stat |= 0x02;
		}
		if (confMgr.getRsb(sensIdx).isIdValid() == true) {
			stat |= 0x04;
		}
		stat |= (uint32_t (confMgr.getRsb(sensIdx).getStatusBits())) << 16;
		Utils::addSerialized(stat, serRes);
	}

	//for each RCB add fields
	for (sensIdx = 0; sensIdx < confMgr.getNumRcbs(); sensIdx++) {
		Utils::addSerialized(uint32_t (confMgr.getRcb(sensIdx).getId()), serRes);
		serRes.push_back(uint8_t (confMgr.getRcb(sensIdx).getAddress()));
		Utils::addSerialized(rcbStat[sensIdx], serRes);
		uint32_t stat = 0x01;
		if (confMgr.getRcb(sensIdx).isAlive() == true) {
			stat |= 0x02;
		}
		if (confMgr.getRcb(sensIdx).isIdValid() == true) {
			stat |= 0x04;
		}
		stat |= (uint32_t (confMgr.getRcb(sensIdx).getStatusBits())) << 16;
		Utils::addSerialized(stat, serRes);
	}
}

void MeasProcessor::initRefTs() {

	unsigned int rsbIdx, measIdx, sensIdx, idx;
	vector<unsigned int> upperRsbIdx;
	vector<unsigned int> upperSensIdx;
	vector<unsigned int> upperNumVldAxle;
	vector<float> upperNumAxAvgDiff;
	vector<unsigned int> lowerRsbIdx;
	vector<unsigned int> lowerSensIdx;
	vector<unsigned int> lowerNumVldAxle;
	vector<float> lowerNumAxAvgDiff;
	vector<unsigned int> numSensorsMeasAx;
	float avgNumAx;

	//clear containers
	upperRefTs100us.clear();
	lowerRefTs100us.clear();
	numRsbMeasAx.clear();

	//iterate over alive RSBs
	for (rsbIdx = 0; rsbIdx < confMgr.getNumRsbs(); rsbIdx++) {
		RsbParameters& currRsb = confMgr.getRsb(rsbIdx);
		if (currRsb.isAlive() == true) {

			//iterate over sensors, init new element in vectors according to sensor position
			numSensorsMeasAx.clear();
			for (sensIdx = 0; sensIdx < currRsb.getNumSens(); sensIdx++) {

				unsigned int* vldAxleCnt;
				if (currRsb.isSensAtUpperEnd(sensIdx) == true) {
					upperRsbIdx.push_back(rsbIdx);
					upperSensIdx.push_back(sensIdx);
					upperNumVldAxle.push_back(0);
					vldAxleCnt = &upperNumVldAxle[upperNumVldAxle.size()-1];
				}
				else {
					lowerRsbIdx.push_back(rsbIdx);
					lowerSensIdx.push_back(sensIdx);
					lowerNumVldAxle.push_back(0);
					vldAxleCnt = &lowerNumVldAxle[lowerNumVldAxle.size()-1];
				}

				//count valid measurement entries for current sensor
				for (measIdx = 0; measIdx < currRsb.getNumMeas(); measIdx++) {
					RsbMeasLdataAnswData& currMeas = currRsb.getSingleMeasFull(measIdx);
					if ((currMeas.isDummy() == false) && (currMeas.isValid(sensIdx) == true) && (currMeas.isUsable(sensIdx) == true)) {
						(*vldAxleCnt)++;
					}
				}

				//save value for later usage
				numSensorsMeasAx.push_back(*vldAxleCnt);

				//if the number of axles for the current sensor happens to be 0, remove vector elements
				if (*vldAxleCnt == 0) {
					if (currRsb.isSensAtUpperEnd(sensIdx) == true) {
						upperRsbIdx.pop_back();
						upperSensIdx.pop_back();
						upperNumVldAxle.pop_back();
					}
					else {
						lowerRsbIdx.pop_back();
						lowerSensIdx.pop_back();
						lowerNumVldAxle.pop_back();
					}
				}
			}

			//save number of measured axles for current RSB for stat
			numRsbMeasAx.push_back(*max_element(numSensorsMeasAx.begin(), numSensorsMeasAx.end()));
		}
		else {
			numRsbMeasAx.push_back(0);
		}
	}

	//throw exception if no valid timestamps exist at any end of the system
	if ((upperRsbIdx.empty() == true) || (lowerRsbIdx.empty() == true)) {
		throw runtime_error("Error: no valid RSB sensor data for timestamp alignment");
	}

	//calculate the average number of measured axles from alive RS sensors
	avgNumAx = (float) accumulate(upperNumVldAxle.begin(), upperNumVldAxle.end(), 0);
	avgNumAx += (float) accumulate(lowerNumVldAxle.begin(), lowerNumVldAxle.end(), 0);
	avgNumAx = avgNumAx/(float (upperNumVldAxle.size() + lowerNumVldAxle.size()));

	//for each valid number of axles, calculate and save the absolute difference from average
	for (idx = 0; idx < upperNumVldAxle.size(); idx++) {
		upperNumAxAvgDiff.push_back(fabs(((float) upperNumVldAxle[idx]) - avgNumAx));
	}
	for (idx = 0; idx < lowerNumVldAxle.size(); idx++) {
		lowerNumAxAvgDiff.push_back(fabs(((float) lowerNumVldAxle[idx]) - avgNumAx));
	}

	unsigned int upperBestVecIdx;
	unsigned int lowerBestVecIdx;

	//RSBs tend to measure more wheels than the existing - if the minimal number of axles equal at the end of the
	//systems, use these as reference
	if (*min_element(upperNumVldAxle.begin(), upperNumVldAxle.end()) == *min_element(lowerNumVldAxle.begin(), lowerNumVldAxle.end())) {
		upperBestVecIdx = min_element(upperNumVldAxle.begin(), upperNumVldAxle.end()) - upperNumVldAxle.begin();
		lowerBestVecIdx = min_element(lowerNumVldAxle.begin(), lowerNumVldAxle.end()) - lowerNumVldAxle.begin();
	}
	else {
		//if not, get axle count indices closest to average and use them as reference
		upperBestVecIdx = min_element(upperNumAxAvgDiff.begin(), upperNumAxAvgDiff.end()) - upperNumAxAvgDiff.begin();
		lowerBestVecIdx = min_element(lowerNumAxAvgDiff.begin(), lowerNumAxAvgDiff.end()) - lowerNumAxAvgDiff.begin();
	}

	//throw exception if there are not enough timestamps for alignment procedure
	if ((upperNumVldAxle[upperBestVecIdx] < 2) || (lowerNumVldAxle[lowerBestVecIdx] < 2)) {
		throw runtime_error("Error: not enough timestamps for alignment procedure");
	}

	//set upper reference timestamps
	RsbParameters& upperRsb = confMgr.getRsb(upperRsbIdx[upperBestVecIdx]);
	for (measIdx = 0; measIdx < upperRsb.getNumMeas(); measIdx++) {
		RsbMeasLdataAnswData& currMeas = upperRsb.getSingleMeasFull(measIdx);
		if ((currMeas.isDummy() == false) && (currMeas.isValid(upperSensIdx[upperBestVecIdx]) == true) && (currMeas.isUsable(upperSensIdx[upperBestVecIdx]) == true)) {
			upperRefTs100us.push_back(currMeas.getTimeStamp100us(upperSensIdx[upperBestVecIdx]));
		}
	}
	upperPos_mm = upperRsb.getSensPositions_mm()[upperSensIdx[upperBestVecIdx]];

	//set lower reference timestamps
	RsbParameters& lowerRsb = confMgr.getRsb(lowerRsbIdx[lowerBestVecIdx]);
	for (measIdx = 0; measIdx < lowerRsb.getNumMeas(); measIdx++) {
		RsbMeasLdataAnswData& currMeas = lowerRsb.getSingleMeasFull(measIdx);
		if ((currMeas.isDummy() == false) && (currMeas.isValid(lowerSensIdx[lowerBestVecIdx]) == true) && (currMeas.isUsable(lowerSensIdx[lowerBestVecIdx]) == true)) {
			lowerRefTs100us.push_back(currMeas.getTimeStamp100us(lowerSensIdx[lowerBestVecIdx]));
		}
	}
	lowerPos_mm = lowerRsb.getSensPositions_mm()[lowerSensIdx[lowerBestVecIdx]];

	//determine train direction
	directionLow2Up = upperRefTs100us[0] > lowerRefTs100us[0] ? true : false;
}

void MeasProcessor::filterRsbEnts() {

	unsigned int measIdx, rsbIdx, sensIdx;

	numRsbUnusableEntity.clear();

	//iterate over each sensor of every RSB
	for(rsbIdx = 0; rsbIdx < confMgr.getNumRsbs(); rsbIdx++) {

		RsbParameters& currRsb = confMgr.getRsb(rsbIdx);
		numRsbUnusableEntity.push_back(0);

		for (sensIdx = 0; sensIdx < currRsb.getNumSens(); sensIdx++) {

			unsigned int unusableEntryCnt = 0;

			//collect each consecutive valid measurement of current sensor
			vector<RsbMeasLdataAnswData*> validMeasVec;
			for (measIdx = 0; measIdx < currRsb.getNumMeas(); measIdx++) {

				RsbMeasLdataAnswData& currMeas = currRsb.getSingleMeasFull(measIdx);
				if ((currMeas.isDummy() == false) && (currMeas.isValid(sensIdx) == true)) {
					validMeasVec.push_back(&currMeas);

				}
			}

			//iterate over valid measurements, check conditions
			for (measIdx = 1; measIdx < validMeasVec.size(); measIdx++) {

				//check timestamp distance of current and next entry, if they are too close, keep only the first one
				if (validMeasVec[measIdx]->getTimeStamp100us(sensIdx) < validMeasVec[measIdx-1]->getTimeStamp100us(sensIdx) + AlgPars::rsbPulseDistLowLim) {
					validMeasVec[measIdx]->setAsUnusable(sensIdx);
					unusableEntryCnt++;
				}
				else {
					//if timestamp distance is ok, check pulse length of previous entry
					if ((validMeasVec[measIdx-1]->getPulseLength100us(sensIdx) <= AlgPars::rsbPulseLengthLowLim) &&  (validMeasVec[measIdx-1]->isUsable(sensIdx) == true)){
						validMeasVec[measIdx-1]->setAsUnusable(sensIdx);
						unusableEntryCnt++;
					}
				}

			}

			//check pulse length of last entry which was not yet performed
			if (validMeasVec.size() > 0) {
				if (validMeasVec[validMeasVec.size()-1]->getPulseLength100us(sensIdx)  <= AlgPars::rsbPulseLengthLowLim) {
					validMeasVec[validMeasVec.size()-1]->setAsUnusable(sensIdx);
					unusableEntryCnt++;
				}
			}

			//log if entries were discarded
			if (unusableEntryCnt > 0) {
				sysLog << "Warning: " << unusableEntryCnt <<  " bad entries found for RSB " << CharAsHex(currRsb.getAddress()) << " sensor " << sensIdx;
				sysLog.writeAsError();
				numRsbUnusableEntity[rsbIdx] += unusableEntryCnt;
			}
		}
	}

}

void MeasProcessor::initMeasEnts(bool doLogging) {

	unsigned int rcbIdx, measIdx;
	vector<unsigned int> pulseLengthErrors;
	unsigned int pulseLengthErrCnt;

	//clear containers
	measEntityAll.clear();
	measEntityLeft.clear();
	measEntityRight.clear();
	measPos_mm.clear();

	//iterate over RCBs
	pulseLengthErrors.resize(confMgr.getNumRcbs(), 0);
	for(rcbIdx = 0; rcbIdx < confMgr.getNumRcbs(); rcbIdx++) {

		RcbParameters& currRcb = confMgr.getRcb(rcbIdx);

		//create container and pointer for measurement entities
		measEntityAll.push_back(vector<MeasEntity>());
		if (rcbIdx % 2 == 0) {
			measEntityLeft.push_back(&(measEntityAll[rcbIdx]));
		}
		else {
			measEntityRight.push_back(&(measEntityAll[rcbIdx]));
		}

		//add sensor position for every pair once
		if (rcbIdx % 2 == 0) {
			measPos_mm.push_back(currRcb.getPosition_mm());
		}

		//skip sensor measurements if not alive
		if (currRcb.isAlive() == false) {
			if (doLogging == true) {
				sysLog << "Error: no data available from sensor " << CharAsHex(currRcb.getAddress());
				sysLog.writeAsError();
			}
			continue;
		}

		//iterate over measurement entries, try creating corresponding entities
		pulseLengthErrCnt = 0;
		for(measIdx = 0; measIdx < currRcb.getNumMeas(); measIdx++) {

			try {
				MeasEntity newEntity(currRcb.getSingleMeasFull(measIdx));
				measEntityAll[rcbIdx].push_back(newEntity);
			}
			catch (PulseLengthExc& currExc) {
				//in case of pulse length errors (frequent event), do not log but count
				pulseLengthErrCnt++;
			}
			catch (exception& currExc) {
				if (doLogging == true) {
					sysLog << "Warning: problem with entry " << measIdx << " of sensor " << CharAsHex(currRcb.getAddress()) << ": " << currExc.what();
					sysLog.writeAsError();
				}
			}
		}

		//save pulse length error counter value for RCB
		pulseLengthErrors[rcbIdx] = pulseLengthErrCnt;
	}

	//log pulse length errors if any
	if ((*max_element(pulseLengthErrors.begin(), pulseLengthErrors.end()) > 0) && (doLogging == true)) {
		sysLog << "Warning: entries with too low pulse length found for RCBs: ";
		for(rcbIdx = 0; rcbIdx < confMgr.getNumRcbs(); rcbIdx++) {
			RcbParameters& currRcb = confMgr.getRcb(rcbIdx);
			if (pulseLengthErrors[rcbIdx] > 0) {
				sysLog << CharAsHex(currRcb.getAddress()) << "(" << pulseLengthErrors[rcbIdx] << ") ";
			}
		}
		sysLog.writeAsError();
	}

}

bool MeasProcessor::calcAxleTs() {

	unsigned int axleIdx, measPosIdx;
	unsigned int numAxles;
	float rsbDist_m;
	float avgtAxle0, avgtAxle1;
	vector<float> accVec, dtVec;
	float a, t, t0, t1, v0, s;
	bool axleSpeedOk;

	//re-init axle data
	axleData.clear();
	numAxles = min(upperRefTs100us.size(), lowerRefTs100us.size());
	axleData.resize(numAxles);

	//calculate axle speed, check range
	axleSpeedOk = true;
	rsbDist_m = (float (upperPos_mm - lowerPos_mm))/1000.0f;
	for(axleIdx = 0; axleIdx < numAxles; axleIdx++) {
		axleData[axleIdx].speed_mps = rsbDist_m/((float (abs((int32_t (upperRefTs100us[axleIdx])) - (int32_t (lowerRefTs100us[axleIdx]))))) / 10000.0f);
		if ((axleData[axleIdx].speed_mps < 0.0f) || (axleData[axleIdx].speed_mps > AlgPars::speedMax_mps)) {
			axleSpeedOk = false;
		}
	}

	//calculating average time [s] between consecutive axles (cca. at the middle of the measurement section),
	//and the average acceleration [m/s2] between two consecutive axles
	if (numAxles > 1) {
		for (axleIdx = 0; axleIdx < numAxles-1; axleIdx++) {

			avgtAxle0 = ((float) (upperRefTs100us[axleIdx] + lowerRefTs100us[axleIdx]))/2.0f;
			avgtAxle1 = ((float) (upperRefTs100us[axleIdx+1] + lowerRefTs100us[axleIdx+1]))/2.0f;
			dtVec.push_back((avgtAxle1 - avgtAxle0)/10000.0f);
			accVec.push_back((axleData[axleIdx+1].speed_mps - axleData[axleIdx].speed_mps)/dtVec[axleIdx]);
		}
	}
	else {
		dtVec.push_back(1.0f);
		accVec.push_back(0.0f);
	}

	//calculate estimated position of axles
	for (axleIdx = 0; axleIdx < numAxles; axleIdx++) {
		if (axleIdx == 0) {
			axleData[axleIdx].pos_mm = 0;
		}
		else {
			axleData[axleIdx].pos_mm = (uint32_t (roundf(axleData[axleIdx].speed_mps * dtVec[axleIdx-1] * 1000))) + axleData[axleIdx-1].pos_mm;
		}
	}

	//one acc value is needed for each axle - determining this by weighting the two "closest" acc values
	//(determined with the speed of the current axle and its neighbours) by the corresponding time differences
	for (axleIdx = 0; axleIdx < numAxles; axleIdx++) {

		//handling special cases
		if (axleIdx == 0) {
			axleData[axleIdx].estAcc_mpss = accVec[0];
		}
		else {
			if (axleIdx == numAxles - 1) {
				axleData[axleIdx].estAcc_mpss = accVec[numAxles-2];
			}
			else {
				axleData[axleIdx].estAcc_mpss = (accVec[axleIdx-1]*dtVec[axleIdx] + accVec[axleIdx]*dtVec[axleIdx-1])/(dtVec[axleIdx] + dtVec[axleIdx-1]);
			}
		}
	}

	//calculating the reference timestamps corresponding to each axle
	for (axleIdx = 0; axleIdx < numAxles; axleIdx++) {

		//capturing acceleration and initial velocity of current axle
		a = axleData[axleIdx].estAcc_mpss;
		t0 = min(lowerRefTs100us[axleIdx], upperRefTs100us[axleIdx]);
		t1 = max(lowerRefTs100us[axleIdx], upperRefTs100us[axleIdx]);
		v0 = axleData[axleIdx].speed_mps - 0.5f*a*(t1-t0)/10000.0;

		//calculating the reference timestamps at each sensor position
		for (measPosIdx = 0; measPosIdx < measPos_mm.size(); measPosIdx++) {

			//distance of sensor from the RSB from where the train arrived
			if (directionLow2Up == true) {
				s = (measPos_mm[measPosIdx] - lowerPos_mm)/1000.0f;
			}
			else {
				s = (upperPos_mm - measPos_mm[measPosIdx])/1000.0f;
			}

			//calculating time [0.1 ms], applying a=0 if necessary
			if (fabs(a) > 0.001f) {
				t = t0 + (sqrt(v0*v0+2*a*s) - v0)/a*10000.0f;
			}
			else {
				t = t0 + s/axleData[axleIdx].speed_mps*10000.0f;
			}

			//storing timestamp value
			axleData[axleIdx].rcbPosTs.push_back((uint32_t) roundf(t));
		}
	}

	return axleSpeedOk;
}

void MeasProcessor::alignMeasEnts() {

	unsigned int rcbIdx, measIdx, axleIdx;
	uint32_t tsDiff;
	uint32_t refTsDiff;
	vector<uint32_t> tsDiffs;
	vector<unsigned int> closestAxleIdx;
	vector<uint32_t> closestAxleTsDiff;
	vector<unsigned int> closestMeasIdx;
	vector<uint32_t> closestMeasTsDiff;
	unsigned int mutualClosestIdx;

	//process RCBs separately
	for (rcbIdx = 0; rcbIdx < measEntityAll.size(); rcbIdx++) {

		vector<MeasEntity>& currEntityVec = measEntityAll[rcbIdx];

		//for each measurement determine closest axle
		closestAxleIdx.clear();
		closestAxleTsDiff.clear();
		for (measIdx = 0; measIdx < currEntityVec.size(); measIdx++) {

			//calculate timestamp distance of current measurement and every axle
			tsDiffs.clear();
			for (axleIdx = 0; axleIdx < axleData.size(); axleIdx++) {
				tsDiff = (uint32_t) abs((int32_t (currEntityVec[measIdx].measEntry->getTimeStamp100us())) - (int32_t (axleData[axleIdx].rcbPosTs[rcbIdx/2])));
				tsDiffs.push_back(tsDiff);
			}

			//store closest axle idx and ts diff for current measurement
			vector<uint32_t>::iterator minElemIt = min_element(tsDiffs.begin(), tsDiffs.end());
			closestAxleIdx.push_back(minElemIt - tsDiffs.begin());
			closestAxleTsDiff.push_back(*minElemIt);
		}

		//iterate over axles again
		for (axleIdx = 0; axleIdx < axleData.size(); axleIdx++) {

			//initialize measurement entity pointer of current RCB in MeasAxle to NULL
			axleData[axleIdx].axleMeasEnts.push_back(NULL);

			//register all measurements whose closest axle is the current one
			closestMeasIdx.clear();
			closestMeasTsDiff.clear();
			for (measIdx = 0; measIdx < currEntityVec.size(); measIdx++) {
				if (closestAxleIdx[measIdx] == axleIdx) {
					closestMeasIdx.push_back(measIdx);
					closestMeasTsDiff.push_back(closestAxleTsDiff[measIdx]);
				}
			}

			//determine which of the measurements (whose closest axle is the current one) is closest to this axle
			if (closestMeasIdx.empty() == false) {

				mutualClosestIdx = min_element(closestMeasTsDiff.begin(), closestMeasTsDiff.end()) - closestMeasTsDiff.begin();
				MeasEntity& assignedEntity = currEntityVec[closestMeasIdx[mutualClosestIdx]];

				//set assignment info in corresponding objects
				assignedEntity.axleIdx = axleIdx;
				assignedEntity.isAssigned = true;
				axleData[axleIdx].axleMeasEnts[rcbIdx] = &assignedEntity;

				//store absolute timestamp difference, calculate and store relative one by choosing
				//the smaller of the differences of the consecutive reference timestamps
				assignedEntity.absTsDiff = abs((int32_t (assignedEntity.measEntry->getTimeStamp100us())) - (int32_t (axleData[axleIdx].rcbPosTs[rcbIdx/2])));
				if (axleIdx == 0) {
					refTsDiff = axleData[1].rcbPosTs[rcbIdx/2] - axleData[0].rcbPosTs[rcbIdx/2];
				}
				else {
					if (axleIdx == axleData.size() - 1) {
						refTsDiff = axleData[axleData.size()-1].rcbPosTs[rcbIdx/2] - axleData[axleData.size()-2].rcbPosTs[rcbIdx/2];
					}
					else {
						refTsDiff = min(axleData[axleIdx].rcbPosTs[rcbIdx/2] - axleData[axleIdx-1].rcbPosTs[rcbIdx/2],
										axleData[axleIdx+1].rcbPosTs[rcbIdx/2] - axleData[axleIdx].rcbPosTs[rcbIdx/2]);
					}
				}
				assignedEntity.relTsDiff = (float (assignedEntity.absTsDiff))/(float (refTsDiff))*100.0f;
			}
		}
	}
}

bool MeasProcessor::evalAlignment(bool evalCondHard) {

	unsigned int axleIdx, axleMeasIdx, rcbPairIdx1, rcbPairIdx2;
	unsigned int rcbIdx, rcbOkCnt;
	vector<unsigned int> numTsBelowLim;
	MeasEntity* measQuad[2][2];
	vector<float> quadTsErrDiffs;
	vector<float> axleTsErrDiffs;
	vector<float> axlesMaxTsErrDiff;

	//calculate per-axle alignment info for checking soft alignment condition
	numTsBelowLim.resize(axleData.size(), 0);
	for (axleIdx = 0; axleIdx < axleData.size(); axleIdx++) {

		MeasAxle& currAxle = axleData[axleIdx];

		//get the number of aligned measurement entities whose timestamp error is below limit
		for (axleMeasIdx = 0; axleMeasIdx < currAxle.axleMeasEnts.size(); axleMeasIdx++) {
			if ((currAxle.axleMeasEnts[axleMeasIdx] != NULL) && (fabs(currAxle.axleMeasEnts[axleMeasIdx]->relTsDiff) < AlgPars::limitTsErr)) {
				numTsBelowLim[axleIdx]++;
			}
		}
	}

	//check soft condition: for each axle the majority of the sensors must have a measurement entity
	//with low timestamp error
	//if condition fails, return false right away
	if (*min_element(numTsBelowLim.begin(), numTsBelowLim.end()) < confMgr.getNumRcbs()*AlgPars::limitFitSensRate/100.f) {
		return false;
	}

	//soft condition succeeded, if hard condition check was not requested, return true
	if (evalCondHard == false) {
		return true;
	}

	//check hard condition - there is a 'soft version' of hard condition: if the majority of the RCBs measured the
	//same number of entities (with normal pulse length) than the current number of axles, do not check the
	//'harder version' but treat alignment OK (since hard version tries to detect if two RSBs missed different
	//members of a neighbouring axle pair, but this may fail due to noisy timestamps and may cause invalid meas
	//for accelerating trains, if there is a substantial number of RCBs which measured the current number of axles,
	//we do not check for hard version of hard condition)
	rcbOkCnt = 0;
	for (rcbIdx = 0; rcbIdx < measEntityAll.size(); rcbIdx++) {
		if (measEntityAll[rcbIdx].size() == axleData.size()) {
			rcbOkCnt++;
		}
	}
	if ((float (rcbOkCnt)) >= (float (confMgr.getNumRcbs()))*AlgPars::limitFitSensRate/100.f) {
		return true;
	}

	//if softer version of hard condition fails, check for hard version
	for (axleIdx = 0; axleIdx < axleData.size(); axleIdx++) {

		MeasAxle& currAxle = axleData[axleIdx];

		//get the maximal timestamp error changes of consecutive sensor pairs (quad)
		axleTsErrDiffs.clear();
		for (axleMeasIdx = 0; axleMeasIdx < currAxle.axleMeasEnts.size()-2; axleMeasIdx+=2) {

			//capture measurement pointers of two neighbouring sensor pairs for simplifying indexing
			measQuad[0][0] = currAxle.axleMeasEnts[axleMeasIdx];
			measQuad[0][1] = currAxle.axleMeasEnts[axleMeasIdx+1];
			measQuad[1][0] = currAxle.axleMeasEnts[axleMeasIdx+2];
			measQuad[1][1] = currAxle.axleMeasEnts[axleMeasIdx+3];

			//iterate over sensor quad, comparing consecutive sensor relative errors
			quadTsErrDiffs.clear();
			for (rcbPairIdx1 = 0; rcbPairIdx1 < 2; rcbPairIdx1++) {
				for (rcbPairIdx2 = 0; rcbPairIdx2 < 2; rcbPairIdx2++) {
					if ((measQuad[0][rcbPairIdx1] != NULL) && (measQuad[1][rcbPairIdx2] != NULL)) {
						quadTsErrDiffs.push_back(fabs(measQuad[0][rcbPairIdx1]->relTsDiff - measQuad[1][rcbPairIdx2]->relTsDiff));
					}
				}
			}

			//save maximal error difference for current quad
			if (quadTsErrDiffs.size() > 0) {
				axleTsErrDiffs.push_back(*max_element(quadTsErrDiffs.begin(), quadTsErrDiffs.end()));
			}
		}

		//save maximal error difference for axle if each quad produced a result (no unalive sensor pairs exist)
		if (axleTsErrDiffs.size() == currAxle.axleMeasEnts.size()/2 - 1) {
			axlesMaxTsErrDiff.push_back(*max_element(axleTsErrDiffs.begin(), axleTsErrDiffs.end()));
		}
	}

	//check hard condition - first of all, each axle must have a maximal error difference value,
	//return false if condition is not fulfilled
	if (axlesMaxTsErrDiff.size() < axleData.size()) {
		return false;
	}

	//all maximal timestamp error difference must be below limit,
	//return false if condition is not fulfilled
	if (*max_element(axlesMaxTsErrDiff.begin(), axlesMaxTsErrDiff.end()) > AlgPars::limitTsErrDiff) {
		return false;
	}

	//all conditions are met, return true
	return true;
}

void MeasProcessor::restoreTs() {

	vector<uint32_t> restoredEnter;
	vector<uint32_t> restoredExit;
	unsigned int currEnterIdx, currExitIdx;
	int32_t currTsDiff, nextTsDiff;
	float deltaTsDiff;

	//capture original references according to train direction
	vector<uint32_t> origEnter = (directionLow2Up == true) ? lowerRefTs100us : upperRefTs100us;
	vector<uint32_t> origExit = (directionLow2Up == true) ? upperRefTs100us : lowerRefTs100us;

	//save first timestamp pair, set indices, calculate difference
	restoredEnter.push_back(origEnter[0]);
	restoredExit.push_back(origExit[0]);
	currEnterIdx = 1;
	currExitIdx = 1;
	currTsDiff = (int32_t (origExit[0])) - (int32_t (origEnter[0]));

	//do algorithm while there are remaining unprocessed original timestamps
	while((currEnterIdx < origEnter.size()) || (currExitIdx < origExit.size())) {

		//if maximal axle number is reached, throw exception
		if (restoredEnter.size() >= Config::numMaxAxles) {
			throw runtime_error("Restoring timestamps failed since maximal axle number was reached");
		}

		//if both original vector has unprocessed timestamps, checking if next one correspond
		//to each other and restore ts if necessary
		if ((currEnterIdx < origEnter.size()) && (currExitIdx < origExit.size())) {

			nextTsDiff = (int32_t (origExit[currExitIdx])) - (int32_t (origEnter[currEnterIdx]));
			deltaTsDiff = (float (nextTsDiff - currTsDiff))/(float (currTsDiff))*100.0f;

			//if the change is below limit, supposedly they correspond to the same axle
			if (fabs(deltaTsDiff) < AlgPars::maxTsDiffChange) {
				restoredEnter.push_back(origEnter[currEnterIdx]);
				restoredExit.push_back(origExit[currExitIdx]);
				currEnterIdx++;
				currExitIdx++;

				//updating ts diff
				currTsDiff = nextTsDiff;
			}
			else {

				//the difference if too high, supposedly one sensor skipped a wheel, restore timestamp
				if (nextTsDiff < currTsDiff) {

					//difference became shorter, the enter one skipped the wheel
					restoredEnter.push_back(uint32_t ((int32_t (origExit[currExitIdx]) - currTsDiff)));
					restoredExit.push_back(origExit[currExitIdx]);
					currExitIdx++;
				}
				else {

					//difference became longer, the exit one skipped the wheel
					restoredEnter.push_back(origEnter[currEnterIdx]);
					restoredExit.push_back(uint32_t ((int32_t (origEnter[currEnterIdx]) + currTsDiff)));
					currEnterIdx++;
				}
			}
		}
		else {

			//if only one of the original vectors has unprocessed timestamps,
			//perform the above restoration step accordingly
			if (currEnterIdx < origEnter.size()) {
				restoredEnter.push_back(origEnter[currEnterIdx]);
				restoredExit.push_back(uint32_t ((int32_t (origEnter[currEnterIdx]) + currTsDiff)));
				currEnterIdx++;
			}
			else {
				restoredEnter.push_back(uint32_t ((int32_t (origExit[currExitIdx]) - currTsDiff)));
				restoredExit.push_back(origExit[currExitIdx]);
				currExitIdx++;
			}
		}
	}

	//save restored timestamps
	lowerRefTs100us = (directionLow2Up == true) ? restoredEnter : restoredExit;
	upperRefTs100us = (directionLow2Up == true) ? restoredExit : restoredEnter;
}

void MeasProcessor::disassignErrEnts() {

	unsigned int axleIdx, measIdx;

	for (axleIdx = 0; axleIdx < axleData.size(); axleIdx++) {
		for (measIdx = 0; measIdx < axleData[axleIdx].axleMeasEnts.size(); measIdx++) {
			MeasEntity* currMeas = axleData[axleIdx].axleMeasEnts[measIdx];
			if ((currMeas != NULL) && (fabs(currMeas->relTsDiff) > AlgPars::limitTsErr)) {
				currMeas->isAssigned = false;
				currMeas->axleIdx = currMeas->measEntry->getAxleId();
				axleData[axleIdx].axleMeasEnts[measIdx] = NULL;
			}
		}
	}
}

void MeasProcessor::assignDefault() {

	unsigned int axleIdx;
	unsigned int rcbIdx;
	vector<unsigned int> numEnts;
	unsigned int numAxles;
	float rsbDist_m;

	//clear axle data containing possibly invalid alingment info
	axleData.clear();

	//determine train direction based on first timestamp values if exist,
	//set default direction if not
	try {
		directionLow2Up = upperRefTs100us.at(0) > lowerRefTs100us.at(0) ? true : false;
	}
	catch (...) {
		directionLow2Up = Config::isDefDirLow2Up;
	}

	//treat the maximal entity number RCBs as number of axles
	for(rcbIdx = 0; rcbIdx < measEntityAll.size(); rcbIdx++) {
		numEnts.push_back(measEntityAll[rcbIdx].size());
	}
	numAxles = *max_element(numEnts.begin(), numEnts.end());

	//create axle data, assign entities sequentially to axles (ignoring original axleIdx field)
	axleData.resize(numAxles);
	for (axleIdx = 0; axleIdx < numAxles; axleIdx++) {

		//determine axle speed based on timestamps if available,
		//set default axle speed if not or if it is outside range
		try {
			rsbDist_m = (float (upperPos_mm - lowerPos_mm))/1000.0f;
			axleData[axleIdx].speed_mps = fabs(rsbDist_m/((float (upperRefTs100us.at(axleIdx) - lowerRefTs100us.at(axleIdx))) / 10000.0f));
			if ((axleData[axleIdx].speed_mps < 0.0f) || (axleData[axleIdx].speed_mps > AlgPars::speedMax_mps)) {
				throw;
			}
		}
		catch(...) {
			axleData[axleIdx].speed_mps = Config::defAxSpeed_kmph/3.6;
		}

		//iterate over RCBs, assign entity at axleIdx index to this axle if available
		for (rcbIdx = 0; rcbIdx < measEntityAll.size(); rcbIdx++) {
			if (axleIdx < measEntityAll[rcbIdx].size()) {
				axleData[axleIdx].axleMeasEnts.push_back(&(measEntityAll[rcbIdx][axleIdx]));
				measEntityAll[rcbIdx][axleIdx].isAssigned = true;
				measEntityAll[rcbIdx][axleIdx].axleIdx = axleIdx;
			}
			else {
				axleData[axleIdx].axleMeasEnts.push_back(NULL);
			}
		}
	}
}

void MeasProcessor::processCurves() {

	unsigned int axleIdx, measIdx;

	//iterate over each assigned measurement entity of every axle
	for (axleIdx = 0; axleIdx < axleData.size(); axleIdx++) {
		for (measIdx = 0; measIdx < axleData[axleIdx].axleMeasEnts.size(); measIdx++) {
			if (axleData[axleIdx].axleMeasEnts[measIdx] != NULL) {

				//get converted curve of current entity
				vector<int16_t> currSamples = axleData[axleIdx].axleMeasEnts[measIdx]->calcConvSamples();
				vector<int> fittedCurve;

				//perform curve fitting for current measurement entity
				fitTheoretical(*(axleData[axleIdx].axleMeasEnts[measIdx]), currSamples, fittedCurve);

				//perform curve evaluation for current measurement entity
				evalCurve(*(axleData[axleIdx].axleMeasEnts[measIdx]), currSamples, fittedCurve);
			}
		}

		//determine if measurement entities can be used for load calculation per-axle
		setIsUsable(axleData[axleIdx]);
	}
}

void MeasProcessor::fitTheoretical(MeasEntity& measEnt, const vector<int16_t>& currSamples, vector<int>& fittedCurve) {

	const float lowLimits[3] = {AlgPars::lsScaleMin, AlgPars::lsPadMin, AlgPars::lsPadMin};
	const float highLimits[3] = {AlgPars::lsScaleMax, AlgPars::lsPadMax, AlgPars::lsPadMax};
	const float randIvInit[3] = {AlgPars::lsRandIntervalScale, AlgPars::lsRandIntervalPad, AlgPars::lsRandIntervalPad};
	const ArithmeticVector lowLimVec(lowLimits, lowLimits+3);
	const ArithmeticVector highLimVec(highLimits, highLimits+3);
	const ArithmeticVector randIvInitVec(randIvInit, randIvInit+3);

	//create curve fitting error object with current samples and limit vectors
	CurveFittingError fitError(currSamples, lowLimVec, highLimVec);

	//create optimizer object with specific parameters
	LSOptimizer lsOpt(randIvInitVec, AlgPars::lsRhoLowerBound, AlgPars::lsMaxNumIters, AlgPars::lsMaxConsSucc, AlgPars::lsMaxConsFail, AlgPars::lsExpansionFactor, AlgPars::lsContractionFactor);

	//create initial parameter vector - init scale is the average of the center part of the curve
	unsigned int numSamp = currSamples.size();
	float initScale = ((float) accumulate(currSamples.begin()+3*numSamp/8, currSamples.begin()+5*numSamp/8, 0)) / (float (numSamp/4));

	ArithmeticVector initParVec;
	initParVec.push_back(initScale);
	initParVec.push_back(0.0f);
	initParVec.push_back(0.0f);

	//perform local search
	lsOpt.doLS(fitError, initParVec);

	//save fitting results in measurement entity object
	const ArithmeticVector& finalPar = lsOpt.getOptPar();
	measEnt.curveParams.fitScale = finalPar[0];
	measEnt.curveParams.fitPadLeft = finalPar[1];
	measEnt.curveParams.fitPadRight = finalPar[2];
	measEnt.curveParams.fitErr2 = lsOpt.getOptScore();
	fittedCurve = fitError.getTransformedCurve();
}

void MeasProcessor::evalCurve(MeasEntity& measEnt, const vector<int16_t>& currSamples, const vector<int>& fittedCurve) {

	unsigned int sampleIdx, shiftIdx;
	vector<int> sampleDiff;
	int minDiffWide, maxDiffWide, minDiffCent, maxDiffCent;
	int total, samplesd2;

	//calculate difference signal between measured and fitted curve
	for (sampleIdx = 0; sampleIdx < currSamples.size(); sampleIdx++) {
		sampleDiff.push_back((int (currSamples[sampleIdx])) - fittedCurve[sampleIdx]);
	}

	//get minimal and maximal difference for all end for the center part of the signal
	minDiffWide = *min_element(sampleDiff.begin(), sampleDiff.end());
	maxDiffWide = *max_element(sampleDiff.begin(), sampleDiff.end());
	minDiffCent = *min_element(sampleDiff.begin() + sampleDiff.size()/4, sampleDiff.begin() + sampleDiff.size()*3/4);
	maxDiffCent = *max_element(sampleDiff.begin() + sampleDiff.size()/4, sampleDiff.begin() + sampleDiff.size()*3/4);

	//set e' values
	measEnt.curveParams.eWide = (float (maxDiffWide - minDiffWide))/measEnt.curveParams.fitScale*100.0f;
	measEnt.curveParams.eCenter = (float (maxDiffCent - minDiffCent))/measEnt.curveParams.fitScale*100.0f;

	//calculate center of mass of measured curve
	total = 0;
	for (sampleIdx = 0; sampleIdx < currSamples.size(); sampleIdx++) {
		total += (sampleIdx+1) * currSamples[sampleIdx];
	}
	measEnt.curveParams.centerOfMass = (float (total)) / (float (accumulate(currSamples.begin(), currSamples.end(), 0)));

	//calculate smoothness of measured curve
	total = 0;
	for (sampleIdx = 0; sampleIdx < currSamples.size()-2; sampleIdx++) {
		samplesd2 = currSamples[sampleIdx] - 2*currSamples[sampleIdx+1] + currSamples[sampleIdx+2];
		total += samplesd2 * samplesd2;
	}
	measEnt.curveParams.smoothness = (float (total)) / (measEnt.curveParams.fitScale * measEnt.curveParams.fitScale) * 100.0f;

	//detect flat hit on signal, but first determine which signal part to use
	//ignore tail parts if padding of fitted curve is high
	vector<int> diffWd;
	int numSampNotUsedDiv2;
	if ((measEnt.curveParams.fitPadLeft > AlgPars::wdPadHighLim) || (measEnt.curveParams.fitPadRight > AlgPars::wdPadHighLim)) {
		diffWd.resize(sampleDiff.size()*3/4);
		copy(sampleDiff.begin() + sampleDiff.size()/8,
				sampleDiff.begin() + sampleDiff.size()*7/8, diffWd.begin());
		numSampNotUsedDiv2 = sampleDiff.size()*1/8;
	}
	else {
		diffWd.resize(sampleDiff.size());
		copy(sampleDiff.begin(), sampleDiff.end(), diffWd.begin());
		numSampNotUsedDiv2 = 0;
	}

	//iterate over different shifts of the error signal
	vector<int> samplesDiffDiff;
	vector<int> monotonCand;
	vector<int> shiftMaxDiffs;
	vector<pair<int, int> > shiftMaxIndices;
	for (shiftIdx = 1; shiftIdx < AlgPars::wdMaxHitFlatWidthSamp32*sampleDiff.size()/32; shiftIdx++) {

		//calculate the difference of the original and shifted difference vector,
		//and check if consecutive samples are monotonic, overwrite difference
		//with 0 if they are not
		samplesDiffDiff.clear();
		monotonCand.resize(shiftIdx+1);
		for (sampleIdx = shiftIdx; sampleIdx < diffWd.size(); sampleIdx++) {
			samplesDiffDiff.push_back(diffWd[sampleIdx] - diffWd[sampleIdx-shiftIdx]);
			copy(diffWd.begin()+sampleIdx-shiftIdx, diffWd.begin()+sampleIdx+1, monotonCand.begin());
			vector<int> monotonCandSorted(monotonCand);
			sort(monotonCandSorted.begin(), monotonCandSorted.end());
			if (monotonCandSorted != monotonCand) {
				samplesDiffDiff[sampleIdx-shiftIdx] = 0;
			}
		}

		//add maximal difference and original indices to vectors
		vector<int>::iterator maxIt = max_element(samplesDiffDiff.begin(), samplesDiffDiff.end());
		shiftMaxDiffs.push_back(*maxIt);
		shiftMaxIndices.push_back(pair<int,int>(maxIt-samplesDiffDiff.begin()+numSampNotUsedDiv2,
											    maxIt-samplesDiffDiff.begin()+shiftIdx+numSampNotUsedDiv2));
	}

	//detect highest monotonic rising edge, save indices and also normalized rising edge value
	vector<int>::iterator maxRisingIt = max_element(shiftMaxDiffs.begin(), shiftMaxDiffs.end());
	measEnt.curveParams.risingSampIdx = shiftMaxIndices[maxRisingIt - shiftMaxDiffs.begin()];
	float rising = *maxRisingIt/measEnt.curveParams.fitScale*100.0f;
	measEnt.curveParams.normRising = (rising < 30.0f) ? 20.0f : ((rising < 40.0f) ? 2.0f*(rising - 30.0f)+20.0f : rising);
}

void MeasProcessor::setIsUsable(MeasAxle& measAx) {

	unsigned int measIdx;
	vector<int32_t> pulseLength;
	int32_t refPulseLength, pulseLengthDiff;

	//iterate over measurement entries seperately, check some parameters, also
	//collect pulse lengh values
	for (measIdx = 0; measIdx < measAx.axleMeasEnts.size(); measIdx++) {

		MeasEntity* currMeasEnt = measAx.axleMeasEnts[measIdx];
		if (currMeasEnt != NULL) {

			//init flag to true
			currMeasEnt->isUsable = true;
			currMeasEnt->isUsableForWd = true;

			//set flag to false if center is out of range
			if ((currMeasEnt->curveParams.centerOfMass < AlgPars::centMassLowLim/100.0f*confMgr.getSniPars().getNumSamples()) ||
				(currMeasEnt->curveParams.centerOfMass > AlgPars::centMassHighLim/100.0f*confMgr.getSniPars().getNumSamples())) {
				currMeasEnt->isUsable = false;
				currMeasEnt->isUsableForWd = false;
			}

			//set flag to false if both wide e' and smoothness is too high, but
			//leave flag indicating usability for wheel diagnostics true
			if ((currMeasEnt->curveParams.eWide > AlgPars::invldELim) &&
				(currMeasEnt->curveParams.smoothness > AlgPars::invldSmoothLim)) {
				currMeasEnt->isUsable = false;
			}

			//save pulse length if entity is usable
			if (currMeasEnt->isUsable == true) {
				pulseLength.push_back(int32_t (uint32_t (currMeasEnt->measEntry->getPulseLength100us())));
			}
		}
	}

	//invalidate samples whose pulse length is very different from the ones assigned
	//to the same axle, but only if alignment was successful
	if (((alOkWoutTsRest == true) || (alOkWithTsRest == true)) && (pulseLength.empty() == false)) {

		//choose the middle one as the reference (hopefully this corresponds to a good pulse)
		sort(pulseLength.begin(), pulseLength.end());
		refPulseLength = pulseLength[pulseLength.size()/2];

		//iterate over measurements again, invalidate entity if pulse length is too different
		for (measIdx = 0; measIdx < measAx.axleMeasEnts.size(); measIdx++) {

			MeasEntity* currMeasEnt = measAx.axleMeasEnts[measIdx];
			if (currMeasEnt != NULL) {

				pulseLengthDiff = refPulseLength - (int32_t (uint32_t (currMeasEnt->measEntry->getPulseLength100us())));
				if (((float) abs(pulseLengthDiff))/refPulseLength > AlgPars::pulseLengthTolerance) {
					currMeasEnt->isUsable = false;
					currMeasEnt->isUsableForWd = false;
				}
			}
		}
	}
}

void MeasProcessor::calcTemps() {

	unsigned int rcbIdx, measIdx;
	float avgTemp;
	bool avgTempVld;
	vector<float> tempVec;

	//clear temperature vector
	rcbAvgTemp.clear();

	//iterate over RCBs for determining average temperature
	for (rcbIdx = 0; rcbIdx < confMgr.getNumRcbs(); rcbIdx++) {

		avgTempVld = false;
		avgTemp = 0.0f;
		if (confMgr.getRcb(rcbIdx).isTempSensor() == true) {

			tempVec.clear();

			//iterate over usable measurements
			vector<MeasEntity>& currMeasVec = measEntityAll[rcbIdx];
			for (measIdx = 0; measIdx < currMeasVec.size(); measIdx++) {
				if (currMeasVec[measIdx].isUsable == true) {
					tempVec.push_back(currMeasVec[measIdx].measEntry->getTempC());
				}
			}

			//calculate and save average temperature
			if (tempVec.size() > 0) {
				avgTemp = accumulate(tempVec.begin(), tempVec.end(), 0.0f)/(float (tempVec.size()));
				avgTempVld = true;
			}
		}

		//save valid flag and temperature
		rcbAvgTemp.push_back(pair<bool, float>(avgTempVld, avgTemp));
	}

	//iterate over RCB average temperatures for determining global average temperature
	tempVec.clear();
	for (rcbIdx = 0; rcbIdx < rcbAvgTemp.size(); rcbIdx++) {
		if (rcbAvgTemp[rcbIdx].first == true) {
			tempVec.push_back(rcbAvgTemp[rcbIdx].second);
		}
	}

	//set average temperature
	if (tempVec.size() > 0) {
		avgTemp = accumulate(tempVec.begin(), tempVec.end(), 0.0f)/(float (tempVec.size()));
		measAvgTemp = avgTemp;
		measAvgTempDefUsed = false;
	}
	else {
		measAvgTemp = Config::defTemperature_C;
		measAvgTempDefUsed = true;
	}
}

void MeasProcessor::scaleHeights() {

	unsigned int rcbIdx, measIdx;
	float offset, scaleSpeed, scaleTemp;
	float speed_kmph, temp_C;
	float scaleFactor;

	//iterate over RCBs
	for (rcbIdx = 0; rcbIdx < confMgr.getNumRcbs(); rcbIdx++) {

		//get RCB parameters and measurements, also scale parameters and sensor temperature
		vector<MeasEntity>& currMeasVec = measEntityAll[rcbIdx];
		RcbParameters& currRcb = confMgr.getRcb(rcbIdx);

		offset = currRcb.getScaleOffset(directionLow2Up);
		scaleSpeed = currRcb.getScaleSpeed(directionLow2Up);
		scaleTemp = currRcb.getScaleTemp(directionLow2Up);

		//TODO: each RCB could use its own temperature value
		temp_C = measAvgTemp;

		//iterate over usable entities
		for (measIdx = 0; measIdx < currMeasVec.size(); measIdx++) {
			if (currMeasVec[measIdx].isUsable == true) {

				//get speed of axle
				speed_kmph = axleData[currMeasVec[measIdx].axleIdx].speed_mps*3.6f;

				//calculate scale factor
				scaleFactor = (scaleSpeed*speed_kmph + scaleTemp*temp_C + offset)*confMgr.getSniPars().getGlobalScale();

				//calculate load
				currMeasVec[measIdx].load_kg = currMeasVec[measIdx].curveParams.fitScale*scaleFactor;
			}
		}
	}
}

void MeasProcessor::calcAxleResults() {

	unsigned int axleIdx, rcbIdx;
	vector<float> leftLoads, rightLoads, axleLoads;
	unsigned int numWheelsUsable;
	vector<float> eCents, eCentsSmooth;
	float worstE, worstESmooth;

	//process each axle separately
	for (axleIdx = 0; axleIdx < axleData.size(); axleIdx++) {

		//get current axle reference
		MeasAxle& currAxle = axleData[axleIdx];

		//collect loads for left and right wheel and full axle where possible
		leftLoads.clear();
		rightLoads.clear();
		axleLoads.clear();
		for (rcbIdx = 0; rcbIdx < currAxle.axleMeasEnts.size(); rcbIdx+=2) {

			numWheelsUsable = 0;

			//get left load if usable
			if ((currAxle.axleMeasEnts[rcbIdx] != NULL) && (currAxle.axleMeasEnts[rcbIdx]->isUsable == true)) {
				leftLoads.push_back(currAxle.axleMeasEnts[rcbIdx]->load_kg);
				numWheelsUsable++;
			}

			//get right load if usable
			if ((currAxle.axleMeasEnts[rcbIdx+1] != NULL) && (currAxle.axleMeasEnts[rcbIdx+1]->isUsable == true)) {
				rightLoads.push_back(currAxle.axleMeasEnts[rcbIdx+1]->load_kg);
				numWheelsUsable++;
			}

			//save sum if both are available
			if (numWheelsUsable == 2) {
				axleLoads.push_back(leftLoads.back()+rightLoads.back());
			}
		}

		//calculate and save average loads and corresponding stddev
		currAxle.loadLeft_kg = doAveraging(leftLoads, false, 3).first;
		currAxle.loadRight_kg = doAveraging(rightLoads, false, 3).first;
		pair<float, float> axleMeanStd = doAveraging(axleLoads, AlgPars::discardExtremums, AlgPars::discardIfAtLeast);
		currAxle.loadAx_kg = axleMeanStd.first;
		currAxle.stdDevLoadAx_kg = axleMeanStd.second;
		currAxle.numUsedSensPairs = axleLoads.size();

		//collect e' values and e' values with smoothness below limit
		eCents.clear();
		eCentsSmooth.clear();
		for (rcbIdx = 0; rcbIdx < currAxle.axleMeasEnts.size(); rcbIdx++) {
			if ((currAxle.axleMeasEnts[rcbIdx] != NULL) && (currAxle.axleMeasEnts[rcbIdx]->isUsable == true)) {

				eCents.push_back(currAxle.axleMeasEnts[rcbIdx]->curveParams.eCenter);
				if (currAxle.axleMeasEnts[rcbIdx]->curveParams.smoothness < AlgPars::dmgSmoothLim) {
					eCentsSmooth.push_back(currAxle.axleMeasEnts[rcbIdx]->curveParams.eCenter);
				}
			}
		}

		//get and save worst e' values, round and saturate if necessary
		worstE = (eCents.size() > 0) ? (*max_element(eCents.begin(), eCents.end())) : 0.0f;
		worstESmooth = (eCentsSmooth.size() > 0) ? (*max_element(eCentsSmooth.begin(), eCentsSmooth.end())) : 0.0f;
		currAxle.worstE = (worstE < 255.0f) ? (uint8_t (roundf(worstE))) : 255;
		currAxle.worstWdE = (worstESmooth < 255.0f) ? (uint8_t (roundf(worstESmooth))) : 255;

		//calculate wheel diagnostic value
		currAxle.wheelDiag = calcAxleWheelDiagVal(currAxle);
	}
}

void MeasProcessor::calcSensorStats() {

	int numOfAxles;
	unsigned int rcbIdx, measIdx, rsbIdx;
	float tempStat;

	//clear status vectors
	rsbStat.clear();
	rcbStat.clear();

	//for each RCB calculate the number of usable (assigned) measurement entries if the measurement is OK,
	//store the number of entries if it is not OK
	numRcbMeasAx.clear();
	for (rcbIdx = 0; rcbIdx < measEntityAll.size(); rcbIdx++) {
		if ((alUsingDef == false) && ((alOkWoutTsRest == true) || (alOkWithTsRest == true))) {
			unsigned int numMeasAxles = 0;
			for (measIdx = 0; measIdx < measEntityAll[rcbIdx].size(); measIdx++) {
				if (measEntityAll[rcbIdx][measIdx].isAssigned == true) {
					numMeasAxles++;
				}
			}
			numRcbMeasAx.push_back(numMeasAxles);
		}
		else {
			numRcbMeasAx.push_back(measEntityAll[rcbIdx].size());
		}
	}

	//determine the number of axles which will be used as a reference
	//if the measurement is OK, it is known, if it is not OK, we suppose that the RSB with
	//the most measurements could measure all axles
	if ((alUsingDef == false) && ((alOkWoutTsRest == true) || (alOkWithTsRest == true))) {
		numOfAxles = axleData.size();
	}
	else {
		numOfAxles = *max_element(numRsbMeasAx.begin(), numRsbMeasAx.end());
	}
	numOfAxles = max(numOfAxles, 1);

	//calculate axle ratio for each RSB and RCB, saturate at 100%
	for (rsbIdx = 0; rsbIdx < numRsbMeasAx.size(); rsbIdx++) {
		tempStat = (float (max(((int) numRsbMeasAx[rsbIdx])-((int) numRsbUnusableEntity[rsbIdx]),0)))/(float (numOfAxles))*100.0f;
		//if stat is higher than 100%, it means unusable entities not detected for an RSB which was not used later (supposing valid meas),
		//so it should be penalitized
		if (tempStat > 100.0f) {
			tempStat = max(100.0f - (tempStat - 100.0f), 0.0f);
		}
		rsbStat.push_back(tempStat);
	}

	//calculate axle ratio for each RCB, saturate at 100%
	for (rcbIdx = 0; rcbIdx < numRcbMeasAx.size(); rcbIdx++) {
		rcbStat.push_back(min((float (numRcbMeasAx[rcbIdx]))/(float (numOfAxles))*100.0f, 100.0f));
	}

}

pair<float, float> MeasProcessor::doAveraging(const vector<float>& values, bool discExtremums, unsigned int discIfAtLeast) {

	unsigned int valIdx;
	vector<float>::const_iterator valIt;
	float mean, stdDev;

	//return in case of empty vector
	if (values.empty() == true) {
		return pair<float, float>(0.0f, 0.0f);
	}

	//collecting the values to be used for averaging (skipping extremums if required)
	vector<float> valuesToProc;
	if ((discExtremums == true) && (values.size() >= discIfAtLeast)) {

		//get extremums
		vector<float>::const_iterator minValIt = min_element(values.begin(), values.end());
		vector<float>::const_iterator maxValIt = max_element(values.begin(), values.end());

		//add non-extremum elements to vector
		for (valIt = values.begin(); valIt != values.end(); valIt++) {
			if ((valIt != minValIt) && (valIt != maxValIt)) {
				valuesToProc.push_back(*valIt);
			}
		}
	}
	else {
		valuesToProc = values;
	}

	//calculate mean and standard deviation
	mean = accumulate(valuesToProc.begin(), valuesToProc.end(), 0.0f)/(float (valuesToProc.size()));
	stdDev = 0.0f;
	for (valIdx = 0; valIdx < valuesToProc.size(); valIdx++) {
		stdDev += (mean - valuesToProc[valIdx]) * (mean - valuesToProc[valIdx]);
	}
	stdDev = sqrt(stdDev/(float (valuesToProc.size())));

	return pair<float, float>(mean, stdDev);
}

float MeasProcessor::calcAxleWheelDiagVal(MeasAxle& measAx) {

	unsigned int rcbIdx, secClusFirstIdx, measIdx;
	vector<float> wheelDiagVals;
	float clusCent1, clusCent2, clusCentCurr;
	float sumOfSquares;
	vector<float> sumOfSquaresVec;
	vector<float> clusCentRatioVec;
	unsigned int optIdx;

	//collect wheel diagnostic values of curves
	for (rcbIdx = 0; rcbIdx < measAx.axleMeasEnts.size(); rcbIdx++) {
		if ((measAx.axleMeasEnts[rcbIdx] != NULL) && (measAx.axleMeasEnts[rcbIdx]->isUsableForWd == true)) {
			wheelDiagVals.push_back(measAx.axleMeasEnts[rcbIdx]->curveParams.normRising);
		}
	}

	//if the number of values is too small, returning default value
	if (wheelDiagVals.size() < 2) {
		return 1.0f;
	}

	//sort values
	sort(wheelDiagVals.begin(), wheelDiagVals.end());

	//calculate k-means value for each potential 2-group clustering
	for (secClusFirstIdx = 1; secClusFirstIdx < wheelDiagVals.size(); secClusFirstIdx++) {

		//calculate cluster centers for current grouping
		clusCent1 = accumulate(wheelDiagVals.begin(), wheelDiagVals.begin()+secClusFirstIdx, 0.0f)/(float (secClusFirstIdx));
		clusCent2 = accumulate(wheelDiagVals.begin()+secClusFirstIdx, wheelDiagVals.end(), 0.0f)/(float (wheelDiagVals.size()-secClusFirstIdx));

		//calculate sum of square of center distances
		sumOfSquares = 0;
		for (measIdx = 0; measIdx < wheelDiagVals.size(); measIdx++) {

			//choose center
			if (measIdx < secClusFirstIdx) {
				clusCentCurr = clusCent1;
			}
			else {
				clusCentCurr = clusCent2;
			}

			sumOfSquares += (wheelDiagVals[measIdx] - clusCentCurr) * (wheelDiagVals[measIdx] - clusCentCurr);
		}

		//save current sum of squares value and corresponding center ratio
		sumOfSquaresVec.push_back(sumOfSquares);
		clusCentRatioVec.push_back(clusCent2/clusCent1);
	}

	//find optimal grouping, return corresponding cluster center ratio
	optIdx = min_element(sumOfSquaresVec.begin(), sumOfSquaresVec.end()) - sumOfSquaresVec.begin();

	return clusCentRatioVec[optIdx];
}

//////////////////////////////////////////////////////////
//		function definitions for class CurveFittingError
//////////////////////////////////////////////////////////

CurveFittingError::CurveFittingError(const vector<int16_t>& convSamples, const ArithmeticVector& paramLowLimInit, const ArithmeticVector& paramHighLimInit) :
	Optimizable(paramLowLimInit, paramHighLimInit),
	samples(convSamples),
	numSamplesDiv2(convSamples.size()/2),
	numSamplesMinus1(convSamples.size() - 1),
	deltaIdx((float (idealCurveSize - 1)) / (float (numSamplesMinus1))),
	numSampTailIgnore((unsigned int) roundf((float (convSamples.size()))*AlgPars::tailIgnoreDuringFit/100.0f)) {

	//create transformed ideal curve vector
	transformedCurve.resize(samples.size(), 0);
}

float CurveFittingError::getScore(const ArithmeticVector& paramVec) {

	unsigned int sampleIdx;
	int roundedShapeIdx;
	float shapeIdx;
	int tempErr;

	//get curve transformation parameters
	float scale = paramVec[0];
	int padLeft = (int) roundf(paramVec[1]);
	int padRight = (int) roundf(paramVec[2]);

	//filling left half of transformed curve
	shapeIdx = float (0 + padLeft);
	for (sampleIdx = 0; sampleIdx < numSamplesDiv2; sampleIdx++) {
		roundedShapeIdx = (int) roundf(shapeIdx);
		if (roundedShapeIdx < 0) {
			transformedCurve[sampleIdx] = (int) roundf(idealCurve[0] * scale);
		}
		else {
			if (roundedShapeIdx > int (leftHalfLastId)) {
				transformedCurve[sampleIdx] = (int) roundf(idealCurve[leftHalfLastId] * scale);
			}
			else {
				transformedCurve[sampleIdx] = (int) roundf(idealCurve[roundedShapeIdx] * scale);
			}
		}
		shapeIdx += deltaIdx;
	}

	//filling right half of transformed curve
	shapeIdx = float (idealCurveSize - 1 - padRight);
	for (sampleIdx = numSamplesMinus1; sampleIdx >= numSamplesDiv2; sampleIdx--) {
		roundedShapeIdx = (int) roundf(shapeIdx);
		if (roundedShapeIdx > int (idealCurveSize - 1)) {
			transformedCurve[sampleIdx] = (int) roundf(idealCurve[idealCurveSize-1] * scale);
		}
		else {
			if (roundedShapeIdx < int (rightHalfFirstId)) {
				transformedCurve[sampleIdx] = (int) roundf(idealCurve[rightHalfFirstId] * scale);
			}
			else {
				transformedCurve[sampleIdx] = (int) roundf(idealCurve[roundedShapeIdx] * scale);
			}
		}
		shapeIdx -= deltaIdx;
	}

	//calculating error (sum of square of errors), excluding two-two samples at the tails
	//TODO: why determine these samples if they are ignored during error calculation?
	sumErr2 = 0;
	for (sampleIdx = numSampTailIgnore; sampleIdx < samples.size() - numSampTailIgnore; sampleIdx++) {
		tempErr = (int (samples[sampleIdx])) - transformedCurve[sampleIdx];
		sumErr2 += float (tempErr * tempErr);
	}

	return sumErr2;
}

int CurveFittingError::getSumErr2() const {
	return sumErr2;
}

const vector<int>& CurveFittingError::getTransformedCurve() const {
	return transformedCurve;
}

//////////////////////////////////////////////////////////
//		stream extraction operators
//////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const vector<int16_t>& obj) {

	unsigned int elementIdx;

	for (elementIdx = 0; elementIdx < obj.size(); elementIdx++) {
		os << setw(6) << obj[elementIdx] << " ";
	}

	return os;
}

ostream& operator<<(ostream& os, const vector<uint32_t>& obj) {

	unsigned int elementIdx;

	for (elementIdx = 0; elementIdx < obj.size(); elementIdx++) {
		os << setw(8) << obj[elementIdx] << " ";
	}

	return os;
}

ostream& operator<<(ostream& os, const MeasEntity& obj) {

	os << "OBJECT: MeasEntity, ";
	os << "Axle ID (orig): " << setw(4) << obj.measEntry->getAxleId() << ", ";
	os << "Axle ID: " << setw(4) << obj.axleIdx << ", ";
	os << "Is assigned: " << setw(1) << obj.isAssigned;

	if (obj.isAssigned == true) {
		os << ", ";
		os << "Abs ts diff: " << setw(8) << obj.absTsDiff << ", ";
		os << "Rel ts diff: " << setw(8) << fixed << setprecision(3) << obj.relTsDiff << ", ";
		os << "Conv samples: " << obj.calcConvSamples() << ", ";
		os << "Fit scale: " << setw(8) << fixed << setprecision(0) << obj.curveParams.fitScale << ", ";
		os << "Pad left: " << setw(4) << fixed << setprecision(0) << obj.curveParams.fitPadLeft << ", ";
		os << "Pad right: " << setw(4) << fixed << setprecision(0) << obj.curveParams.fitPadRight << ", ";
		os << "Fit err2: " << setw(6) << fixed << setprecision(0) << obj.curveParams.fitErr2 << ", ";
		os << "E wide: " << setw(6) << fixed << setprecision(2) << obj.curveParams.eWide << ", ";
		os << "E cent: " << setw(6) << fixed << setprecision(2) << obj.curveParams.eCenter << ", ";
		os << "Center: " << setw(6) << fixed << setprecision(2) << obj.curveParams.centerOfMass << ", ";
		os << "Smoothness: " << setw(6) << fixed << setprecision(2) << obj.curveParams.smoothness << ", ";
		os << "Rising: " << setw(6) << fixed << setprecision(2) << obj.curveParams.normRising << ", ";
		os << "Is usable: " << setw(1) << obj.isUsable << ", ";
		os << "Is usable for wd: " << setw(1) << obj.isUsableForWd << ", ";
		os << "Load: " << setw(6) << fixed << setprecision(0) << obj.load_kg;
	}

	os << endl;

	return os;
}

ostream& operator<<(ostream& os, const MeasAxle& obj) {

	unsigned int measIdx;

	os << "OBJECT: MeasAxle, ";
	os << "Load left: " << setw(5) << fixed << setprecision(0) << obj.loadLeft_kg << ", ";
	os << "Load righ: " << setw(5) << fixed << setprecision(0) << obj.loadRight_kg << ", ";
	os << "Load axle: " << setw(5) << fixed << setprecision(0) << obj.loadAx_kg << ", ";
	os << "Std dev: " << setw(5) << fixed << setprecision(0) << obj.stdDevLoadAx_kg << ", ";
	os << "Num senspair: " << setw(2) << ((unsigned int) (obj.numUsedSensPairs)) << ", ";
	os << "Worst e: " << setw(3) << ((unsigned int) (obj.worstE)) << ", ";
	os << "Worst e_wd: " << setw(3) << ((unsigned int) (obj.worstWdE)) << ", ";
	os << "Wheel diag: " << setw(5) << fixed << setprecision(2) << obj.wheelDiag << ", ";
	os << "Ref timestamps: " << obj.rcbPosTs << ", ";
	os << "Speed m/s: " << setw(5) << fixed << setprecision(2) << obj.speed_mps << ", ";
	os << "Est acc m/s2: " << setw(6) << fixed << setprecision(2) << obj.estAcc_mpss << ", ";
	os << "Pos mm: " << setw(6) << obj.pos_mm << ", ";
	os << "Original axle idx of assigned entities (in the order of RCBs): ";
	for (measIdx = 0; measIdx < obj.axleMeasEnts.size(); measIdx++) {
		if (obj.axleMeasEnts[measIdx] != NULL) {
			os << setw(3) << obj.axleMeasEnts[measIdx]->measEntry->getAxleId() << " ";
		}
		else {
			os << "--- ";
		}
	}

	os << endl;

	return os;
}

ostream& operator<<(ostream& os, const MeasProcessor& obj) {

	unsigned int /*rcbIdx, rsbIdx, measIdx, */axleIdx;

/*	os << "--------------------" << endl;
	os << "RCB MEASUREMENT DATA" << endl;
	os << "--------------------" << endl << endl;

	for (rcbIdx = 0; rcbIdx < obj.measEntityAll.size(); rcbIdx++) {

		os << "Valid measurement entries from RCB " << CharAsHex(obj.confMgr.getRcb(rcbIdx).getAddress()) << ":" << endl;
		for (measIdx = 0; measIdx < obj.measEntityAll[rcbIdx].size(); measIdx++) {
			os << obj.measEntityAll[rcbIdx][measIdx];
		}
		os << endl;
	}*/

/*	os << "------------------------" << endl;
	os << "REFERENCE TIMESTAMP DATA" << endl;
	os << "------------------------" << endl << endl;
	os << "Timestamp alignment status: ";
	if (obj.alUsingDef == true) {
		os << "Alignment procedure could not been performed, default assignment is used";
	}
	else {
		if (obj.alOkWoutTsRest == true) {
			os << "Alignment successful with hard condition";
		}
		else {
			if (obj.alOkWithTsRest == true) {
				os << "Alignment successful with soft condition";
			}
			else {
				os << "Alignment procedure was performed but it was unsuccessful";
			}
		}
	}
	os << endl;
	os << "RSB sensor positions used as reference timestamps (upper, lower): " << obj.upperPos_mm << ", " << obj.lowerPos_mm << endl;
	os << "Reference timestamps of upper sensor: " << obj.upperRefTs100us << endl;
	os << "Reference timestamps of lower sensor: " << obj.lowerRefTs100us << endl << endl;
*/
/*	os << "---------------------------" << endl;
	os << "TEMPERATURE AND STATUS DATA" << endl;
	os << "---------------------------" << endl << endl;
	os << "Per-RCB average temperatures (in the order of RCBs) (valid, avgTemp): ";
	for (rcbIdx = 0; rcbIdx < obj.rcbAvgTemp.size(); rcbIdx++) {
		os << "(" << obj.rcbAvgTemp[rcbIdx].first << ", " << setw(5) << fixed << setprecision(1) << obj.rcbAvgTemp[rcbIdx].second << ") ";
	}
	os << "Measurement average temperature: " <<  setw(5) << fixed << setprecision(1) << obj.measAvgTemp << endl;
	os << "Sensor status: " << endl;
	for (rcbIdx = 0; rcbIdx < obj.confMgr.getNumRcbs(); rcbIdx++) {
		os << "sensor " << CharAsHex(obj.confMgr.getRcb(rcbIdx).getAddress()) << " " << obj.rcbStat[rcbIdx] << "%" << endl;
	}
	for (rsbIdx = 0; rsbIdx < obj.confMgr.getNumRsbs(); rsbIdx++) {
		os << "sensor " << CharAsHex(obj.confMgr.getRsb(rsbIdx).getAddress()) << " " << obj.rsbStat[rsbIdx] << "%" << endl;
	}
	os << endl;*/

	os << "----------------" << endl;
	os << "AXLE RESULT DATA" << endl;
	os << "----------------" << endl << endl;
	for (axleIdx = 0; axleIdx < obj.axleData.size(); axleIdx++) {
		os << "AXLE " << setw(3) << axleIdx << ": " << obj.axleData[axleIdx];
	}
	os << endl << "Train direction is from low to up: " << obj.directionLow2Up << endl;

	return os;
}

//ideal curve and corresponding parameters
const unsigned int CurveFittingError::idealCurveSize = 1000;
const unsigned int CurveFittingError::leftHalfLastId = 499;
const unsigned int CurveFittingError::rightHalfFirstId = 500;
const float CurveFittingError::idealCurve[1000] = {
		0.024334, 0.024351, 0.024404, 0.024492, 0.024615,
		0.024772, 0.024963, 0.025188, 0.025446, 0.025738,
		0.026063, 0.026421, 0.026811, 0.027233, 0.027687,
		0.028173, 0.028691, 0.029239, 0.029819, 0.030429,
		0.031069, 0.031739, 0.032439, 0.033169, 0.033928,
		0.034716, 0.035532, 0.036377, 0.037250, 0.038151,
		0.039079, 0.040035, 0.041018, 0.042028, 0.043064,
		0.044126, 0.045214, 0.046329, 0.047468, 0.048633,
		0.049822, 0.051036, 0.052274, 0.053537, 0.054823,
		0.056133, 0.057466, 0.058822, 0.060200, 0.061601,
		0.063024, 0.064470, 0.065936, 0.067424, 0.068933,
		0.070463, 0.072013, 0.073584, 0.075175, 0.076785,
		0.078415, 0.080063, 0.081731, 0.083417, 0.085122,
		0.086845, 0.088585, 0.090343, 0.092119, 0.093911,
		0.095720, 0.097545, 0.099387, 0.101244, 0.103117,
		0.105006, 0.106909, 0.108827, 0.110760, 0.112707,
		0.114668, 0.116643, 0.118631, 0.120632, 0.122646,
		0.124673, 0.126712, 0.128763, 0.130826, 0.132901,
		0.134987, 0.137083, 0.139191, 0.141309, 0.143437,
		0.145575, 0.147723, 0.149879, 0.152096, 0.154547,
		0.157240, 0.160172, 0.163336, 0.166729, 0.170344,
		0.174177, 0.178222, 0.182474, 0.186929, 0.191581,
		0.196425, 0.201456, 0.206669, 0.212058, 0.217619,
		0.223347, 0.229236, 0.235281, 0.241478, 0.247820,
		0.254304, 0.260923, 0.267673, 0.274548, 0.281544,
		0.288656, 0.295877, 0.303204, 0.310631, 0.318153,
		0.325764, 0.333461, 0.341237, 0.349087, 0.357007,
		0.364991, 0.373034, 0.381131, 0.389277, 0.397467,
		0.405696, 0.413958, 0.422249, 0.430563, 0.438895,
		0.447241, 0.455594, 0.463951, 0.472305, 0.480652,
		0.488987, 0.497304, 0.505598, 0.513865, 0.522099,
		0.530295, 0.538447, 0.546552, 0.554603, 0.562596,
		0.570525, 0.578386, 0.586173, 0.593881, 0.601505,
		0.609040, 0.616481, 0.623823, 0.631060, 0.638187,
		0.645200, 0.652093, 0.658862, 0.665500, 0.672003,
		0.678366, 0.684584, 0.690651, 0.696563, 0.702314,
		0.707900, 0.713314, 0.718553, 0.723610, 0.728481,
		0.733161, 0.737645, 0.741927, 0.746002, 0.749866,
		0.753513, 0.756937, 0.760135, 0.763141, 0.766112,
		0.769062, 0.771991, 0.774900, 0.777789, 0.780657,
		0.783505, 0.786331, 0.789137, 0.791923, 0.794687,
		0.797431, 0.800153, 0.802855, 0.805535, 0.808195,
		0.810833, 0.813450, 0.816046, 0.818621, 0.821174,
		0.823706, 0.826216, 0.828705, 0.831172, 0.833618,
		0.836042, 0.838444, 0.840825, 0.843184, 0.845520,
		0.847835, 0.850128, 0.852399, 0.854648, 0.856874,
		0.859079, 0.861261, 0.863421, 0.865559, 0.867674,
		0.869766, 0.871837, 0.873884, 0.875909, 0.877912,
		0.879891, 0.881848, 0.883782, 0.885693, 0.887582,
		0.889447, 0.891289, 0.893108, 0.894904, 0.896677,
		0.898427, 0.900153, 0.901856, 0.903536, 0.905192,
		0.906825, 0.908434, 0.910019, 0.911581, 0.913119,
		0.914634, 0.916124, 0.917591, 0.919034, 0.920453,
		0.921848, 0.923218, 0.924565, 0.925887, 0.927186,
		0.928460, 0.929709, 0.930934, 0.932135, 0.933311,
		0.934463, 0.935590, 0.936693, 0.937771, 0.938824,
		0.939852, 0.940855, 0.941834, 0.942787, 0.943716,
		0.944619, 0.945498, 0.946351, 0.947183, 0.948011,
		0.948837, 0.949661, 0.950482, 0.951301, 0.952118,
		0.952932, 0.953744, 0.954552, 0.955358, 0.956160,
		0.956959, 0.957755, 0.958547, 0.959336, 0.960121,
		0.960902, 0.961679, 0.962452, 0.963221, 0.963985,
		0.964745, 0.965500, 0.966251, 0.966996, 0.967737,
		0.968472, 0.969203, 0.969928, 0.970647, 0.971361,
		0.972069, 0.972772, 0.973468, 0.974158, 0.974842,
		0.975520, 0.976191, 0.976855, 0.977513, 0.978164,
		0.978808, 0.979445, 0.980075, 0.980697, 0.981312,
		0.981919, 0.982519, 0.983111, 0.983695, 0.984270,
		0.984838, 0.985397, 0.985948, 0.986490, 0.987023,
		0.987548, 0.988063, 0.988570, 0.989067, 0.989555,
		0.990034, 0.990503, 0.990962, 0.991411, 0.991851,
		0.992280, 0.992699, 0.993108, 0.993507, 0.993895,
		0.994272, 0.994638, 0.994993, 0.995338, 0.995671,
		0.995993, 0.996303, 0.996602, 0.996889, 0.997164,
		0.997428, 0.997679, 0.997918, 0.998145, 0.998359,
		0.998561, 0.998750, 0.998927, 0.999090, 0.999241,
		0.999378, 0.999502, 0.999612, 0.999709, 0.999792,
		0.999862, 0.999917, 0.999959, 0.999986, 0.999999,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 1.000000, 1.000000, 1.000000,
		1.000000, 1.000000, 0.999995, 0.999978, 0.999949,
		0.999909, 0.999857, 0.999794, 0.999720, 0.999634,
		0.999537, 0.999429, 0.999311, 0.999181, 0.999041,
		0.998890, 0.998728, 0.998556, 0.998373, 0.998180,
		0.997977, 0.997764, 0.997541, 0.997307, 0.997064,
		0.996811, 0.996549, 0.996276, 0.995994, 0.995703,
		0.995403, 0.995093, 0.994774, 0.994446, 0.994109,
		0.993763, 0.993409, 0.993045, 0.992673, 0.992293,
		0.991904, 0.991506, 0.991101, 0.990687, 0.990265,
		0.989835, 0.989397, 0.988952, 0.988499, 0.988038,
		0.987569, 0.987093, 0.986610, 0.986119, 0.985622,
		0.985117, 0.984605, 0.984086, 0.983560, 0.983028,
		0.982489, 0.981943, 0.981391, 0.980833, 0.980268,
		0.979697, 0.979120, 0.978537, 0.977948, 0.977353,
		0.976752, 0.976146, 0.975534, 0.974917, 0.974294,
		0.973666, 0.973032, 0.972394, 0.971751, 0.971102,
		0.970449, 0.969791, 0.969128, 0.968461, 0.967789,
		0.967112, 0.966432, 0.965747, 0.965058, 0.964365,
		0.963668, 0.962967, 0.962262, 0.961554, 0.960842,
		0.960126, 0.959407, 0.958685, 0.957960, 0.957231,
		0.956499, 0.955763, 0.954994, 0.954182, 0.953327,
		0.952431, 0.951492, 0.950513, 0.949492, 0.948431,
		0.947330, 0.946190, 0.945010, 0.943792, 0.942535,
		0.941240, 0.939908, 0.938538, 0.937132, 0.935690,
		0.934212, 0.932698, 0.931150, 0.929567, 0.927949,
		0.926298, 0.924614, 0.922897, 0.921147, 0.919365,
		0.917552, 0.915707, 0.913832, 0.911926, 0.909990,
		0.908025, 0.906030, 0.904007, 0.901955, 0.899876,
		0.897769, 0.895635, 0.893475, 0.891288, 0.889076,
		0.886838, 0.884575, 0.882288, 0.879976, 0.877641,
		0.875283, 0.872902, 0.870498, 0.868072, 0.865625,
		0.863157, 0.860668, 0.858158, 0.855629, 0.853080,
		0.850512, 0.847925, 0.845320, 0.842698, 0.840057,
		0.837400, 0.834727, 0.832037, 0.829331, 0.826611,
		0.823875, 0.821125, 0.818360, 0.815582, 0.812791,
		0.809988, 0.807171, 0.804343, 0.801503, 0.798652,
		0.795791, 0.792919, 0.790037, 0.787146, 0.784246,
		0.781337, 0.778420, 0.775495, 0.772563, 0.769624,
		0.766679, 0.763727, 0.760770, 0.757807, 0.754840,
		0.751868, 0.748892, 0.745913, 0.742930, 0.739945,
		0.736958, 0.733968, 0.730977, 0.727981, 0.724917,
		0.721764, 0.718523, 0.715197, 0.711786, 0.708293,
		0.704720, 0.701068, 0.697339, 0.693535, 0.689657,
		0.685708, 0.681688, 0.677601, 0.673447, 0.669228,
		0.664947, 0.660604, 0.656202, 0.651743, 0.647227,
		0.642657, 0.638036, 0.633363, 0.628642, 0.623873,
		0.619060, 0.614202, 0.609303, 0.604364, 0.599387,
		0.594373, 0.589325, 0.584243, 0.579130, 0.573988,
		0.568818, 0.563622, 0.558402, 0.553159, 0.547896,
		0.542614, 0.537315, 0.532000, 0.526672, 0.521331,
		0.515981, 0.510622, 0.505257, 0.499887, 0.494514,
		0.489139, 0.483765, 0.478393, 0.473025, 0.467663,
		0.462309, 0.456963, 0.451629, 0.446307, 0.441000,
		0.435709, 0.430436, 0.425183, 0.419951, 0.414743,
		0.409560, 0.404404, 0.399276, 0.394179, 0.389113,
		0.384082, 0.379086, 0.374128, 0.369209, 0.364330,
		0.359495, 0.354703, 0.349958, 0.345261, 0.340614,
		0.336018, 0.331475, 0.326988, 0.322557, 0.318184,
		0.313872, 0.309622, 0.305435, 0.301315, 0.297261,
		0.293277, 0.289363, 0.285522, 0.281755, 0.278064,
		0.274452, 0.270918, 0.267467, 0.264098, 0.260814,
		0.257617, 0.254508, 0.251490, 0.248522, 0.245561,
		0.242608, 0.239662, 0.236725, 0.233796, 0.230875,
		0.227963, 0.225060, 0.222166, 0.219282, 0.216407,
		0.213543, 0.210688, 0.207844, 0.205010, 0.202187,
		0.199376, 0.196575, 0.193786, 0.191009, 0.188244,
		0.185491, 0.182750, 0.180023, 0.177308, 0.174606,
		0.171918, 0.169244, 0.166583, 0.163936, 0.161304,
		0.158686, 0.156083, 0.153496, 0.150923, 0.148366,
		0.145825, 0.143300, 0.140791, 0.138298, 0.135822,
		0.133363, 0.130921, 0.128497, 0.126090, 0.123701,
		0.121330, 0.118977, 0.116643, 0.114328, 0.112032,
		0.109755, 0.107498, 0.105260, 0.103042, 0.100844,
		0.098667, 0.096511, 0.094375, 0.092261, 0.090168,
		0.088096, 0.086047, 0.084019, 0.082014, 0.080031,
		0.078071, 0.076134, 0.074220, 0.072330, 0.070464,
		0.068621, 0.066803, 0.065008, 0.063239, 0.061495,
		0.059775, 0.058081, 0.056413, 0.054770, 0.053153,
		0.051563, 0.049999, 0.048462, 0.046952, 0.045469,
		0.044014, 0.042586, 0.041186, 0.039815, 0.038471,
		0.037157, 0.035871, 0.034614, 0.033387, 0.032189,
		0.031021, 0.029883, 0.028775, 0.027698, 0.026651
};

}

