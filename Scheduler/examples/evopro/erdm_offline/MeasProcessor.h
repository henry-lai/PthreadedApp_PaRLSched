/*
 * MeasProcessor.h
 *
 *  Created on: Sep 10, 2014
 *      Author: pechan
 */

#ifndef ERDM_PC_MEASPROCESSOR_H_
#define ERDM_PC_MEASPROCESSOR_H_

#include <list>

#include "ConfigManager.h"
#include "Logger.h"
#include "LSOptimizer.h"

using namespace sniutils;

namespace sni {

//class for storing algorithm constants
class AlgPars {
public:
	static const bool discardExtremums;
	static const unsigned int discardIfAtLeast;
	static const float lsRhoLowerBound;
	static const unsigned int lsMaxNumIters;
	static const unsigned int lsMaxConsSucc;
	static const unsigned int lsMaxConsFail;
	static const float lsExpansionFactor;
	static const float lsContractionFactor;
	static const float lsRandIntervalScale;
	static const float lsRandIntervalPad;
	static const float lsPadMin;
	static const float lsPadMax;
	static const float lsScaleMin;
	static const float lsScaleMax;
	static const unsigned int pulseLengthLowLim;
	static const unsigned int rsbPulseLengthLowLim;
	static const unsigned int rsbPulseDistLowLim;
	static const float pulseLengthTolerance;
	static const float centMassLowLim;
	static const float centMassHighLim;
	static const float invldELim;
	static const float invldSmoothLim;
	static const float dmgSmoothLim;
	static const float limitTsErr;
	static const float limitTsErrDiff;
	static const float limitFitSensRate;
	static const float maxTsDiffChange;
	static const float wdPadHighLim;
	static const unsigned int wdMaxHitFlatWidthSamp32;
	static const float speedMax_mps;
	static const float tailIgnoreDuringFit;

private:
	AlgPars();
};

//Exception class indicating pulse length problem of measurement curves
class PulseLengthExc: public runtime_error {
public:
	PulseLengthExc(const string& whatMsg) : runtime_error(whatMsg) {}
};

//functor for converting signed measurement samples
class ConvSamples {
public:
	ConvSamples(int16_t offsetInit) : offset(offsetInit) {}
	int16_t operator()(int16_t orig) {
		int16_t diff = orig - offset;
		return (diff < 0 ? (-1*diff) : diff);
	}
private:
	int16_t offset;
};

//class for storing measurement load curve fitting results and parameters
//class will be used only by MeasProcessor - using public fields for ease of access
struct CurveFeatures {
	float fitScale;
	float fitPadLeft;
	float fitPadRight;
	float fitErr2;
	float eWide;
	float eCenter;
	float centerOfMass;
	float smoothness;
	float normRising;
	pair<int,int> risingSampIdx;
};

//class for handling measurement entities and storing parameters determined
//during processing the measurement entities
//class will be used only by MeasProcessor - using public fields for ease of access
//	- constructor sets axleIdx based on the provided measEntryInit and sets flag isAssigned
//	  to false, it also initializes convSamples by handing sample signedness
//	  throws PulseLengthExc in case of non-usable measurement entry with pulse length problem
//	  throws exception in case of non-usable measurement entry (invalid or missing entry)
//	- calcConvSamples calculates and returns transformed curve samples
//	- calcFittedCurve returns fitted curve samples, returns valid result only if curveParams
//	  member stores valid values
struct MeasEntity {
	MeasEntity(RcbMeasLdataAnswData& measEntryInit);
	vector<int16_t> calcConvSamples() const;	//returns converted measurement samples
	vector<int> calcFittedCurve() const;		//returns fitted curve samples
	RcbMeasLdataAnswData* measEntry;	//pointer to measurement entry
	unsigned int axleIdx;				//index of axle which the entry supposedly corresponds to
	bool isAssigned;					//shows if the entry actually was assigned to an axle
	int32_t absTsDiff;					//if assigned (and alignment was successful), stores the difference of the timestamp and the reference
	float relTsDiff;					//if assigned (and alignment was successful), stores the relative difference (compared to reference ts diffs)
	CurveFeatures curveParams;			//if assigned, stores curve features
	bool isUsable;						//if assigned, stores if measurement entity can be used for calculating load
	bool isUsableForWd;					//flag indicating that entity can be used for wheel diagnostic (even if not usable for load calculation)
	float load_kg;						//if usable (which means that also assigned), stores load represented by curve
	friend ostream& operator<<(ostream& os, const MeasEntity& obj);
};

//class for handling axle-specific data during processing the measurement entities
//class will be used only by MeasProcessor - using public fields for ease of access
struct MeasAxle {
	vector<uint32_t> rcbPosTs;			//reference timestamp values for RCB positions
	float speed_mps;					//speed of axle
	float estAcc_mpss;					//estimated acceleration of axle
	vector<MeasEntity*> axleMeasEnts;	//one measurement entity from each RCB assigned to the axle
										//NULL indicates a missing entry for the RCB
	float loadLeft_kg;					//left wheel load
	float loadRight_kg;					//right wheel load
	float loadAx_kg;					//axle load
	float stdDevLoadAx_kg;				//axle load std
	uint8_t numUsedSensPairs;			//number of sensor pairs with data available
	uint8_t worstE;						//worst e' of measurement entires
	uint8_t worstWdE;					//worst e' of measurement entires with smooth curve
	float wheelDiag;					//axle wheel diag value
	uint32_t pos_mm;					//axle position

	friend ostream& operator<<(ostream& os, const MeasAxle& obj);
};

//class for processing measurements
//	- constructor inits confMgr and sysLog members
//	- doProcessing() performs the whole measurement processing algorithm, it sets
//	  all internal members (exception: if the alUsingDef flag is set and default
//	  axle assignment was used, the following fields are not valid: xxxRefTs100us,
//	  absTsDiff and relTsDiff members of MeasEntity objects, rcbPosTs and estAcc_mpss
//	  members of MeasAxle objects)
//	  it does not throw exceptions but logs errors
//	- getSerializedResult() fills the provided byte vector with the results
//	  does not throw exceptions
//	  multi-byte values are stored MSB first, arrays are stored in the order of elements,
//	  struct members are stored in the specified order
//	  the type and order of the values are as follows:
//	  	<uint64_t timestamp>			(timestamp of measurement)
//	  	<uint16_t speedFirst>			(speed of first axle in kmph)
//	  	<uint16_t speedLast>			(speed of last axle in kmph
//	  	<uint32_t measStatus>			(bit0: direction (1 means train passed from lower RSB positions to higher positions))
//										(bit1: measurement valid (1 means it is valid))
//										(bit2: axle number error (1 means error, in this case measurement should be treated as invalid even if bit1 is 1))
//										(bit3: temperature error (1 means temperature is invalid))
//		<float temperature>				(measurement temperature in C degrees)
//		<uint16_t numOfAxles>			(number of axles, indicates size of the following per-axle arrays)
//		<uint16_t leftLoad[]>			(left wheel load for each axle in kg)
//		<uint16_t rightLoad[]>			(right wheel load for each axle in kg)
//		<uint16_t axleLoad[]>			(axle load for each axle in kg)
//		<uint16_t confStdDef[]>			(standard deviation of axle measurement samples for each axle in kg)
//	 	<uint8_t numUsedSensPairs[]>	(number of usable sensor pairs for each axle, used for determining the reliability of measurement and potential alerts)
//		<uint8_t worstE[]>				(old "worst e'" wheel diagnostics value for each axle)
//		<uint8_t worstWdE[]>			(old "worst wd e'" wheel diagnostics value for each axle)
//		<float wheelDiagVal[]>			(new wheel diagnostics value for each axle)
//		<uint16_t numOfSensors>			(number of sensors (RCBs+RSBs), indicates size of the following per-sensor arrays)
//		<sensdata_t sensStats[]>		(sensor status info stored in a struct for each sensor
//			<uint32_t ID>				(ID of sensor)
//			<uint8_t address>			(address of sensor)
//			<float measAxleRatio>		(percentage of the measured axles vs the total number of axles for the sensor)
//			<uint8_t status>			(bit0: RCB/RSB (1 if the sensor is an RCB)
//										(bit1: alive (1 if the sensor was alive during this measurement)
//
//private:
//	- initRefTstamp() gets the initial timestamp reference values, it initializes
//	  upperRefTs100us, lowerRefTs100us, upperPos_mm and lowerPos_mm members based
//	  on confMgr by using the data of those upper&lower sensors which have the most
//	  axle measurements
//	  also set numRsbMeasAx storing the number of original valid measurement entires
//	  for each RSB
//	  throw exception if alignment is not possible (not enough timestamp data for
//	  later steps of the algorithm) and default axle assignment should be used
//	- initMeasEnts() initializes measEntityAll, measEntityLeft, measEntityRight
//	  and measPos_mm fields based on confMgr
//	  measEntityAll stores for each RCB (outer vector) the valid measurement entities
//	  (inner vector), measEntityLeft and Right stores pointers to the inner vectors
//	  for the left and right sensors, measPos_mm stores the position of each pair of
//	  sensors (indexing is consistent, RCB 0 and 1 correspond to position idx 0, etc.)
//	  does not throw exceptions but logs errors in case of invalid or missing entities
//	- calcAxleTs() re-initializes axleData and directionLow2Up members based on the
//	  reference timestamps (xxxRefTs100us) and the RCB positions stored in confMgr
//	  it creates a MeasAxle object for each axle (num of axles equals the size of the
//	  shorter reference timestamp vector) and initializes rcbPosTs, speed_mps and
//	  estAcc_mpss fields
//	  it requires that the reference timestamp vector sizes equal, if they do not,
//	  it uses the smaller size for axle number
//	  it also requires at least one measurement per reference vector
//	- alignMeasEnts() aligns measurement entries to RSB reference timestamps
//	  it sets the pointers in axleMeasEnts member of each MeasAxle object to the entry,
//	  also sets the axleIdx and isAssigned members of the measEntity objects which
//	  could be assigned to an axle, and calculates the absTsDiff and relTsDiff fields
//	- evalAlignment() evaluates the alignment according to the requested condition and
//	  returns true if condition is fulfilled, based on the aligned axleData content
//	  generated by alignMeasEnts()
//	  it requires at least two measurement per reference vector
//	- restoreTs() processes upperRefTs100us and lowerRefTs100us and updates them with
//	  the restored timestamps by trying to detect skipped wheels and calculate missing ts
//	  it requires at least one measurement per reference vector
//	  throws exception if maximal number of axles allowed was reached, original refrerene
//	  timestamps are not altered in this case
//	- disassignErrEnts() disassigns all MeasAxle - MeasEntity assignments with relative
//	  timestamp error above limit, that is, it switches pointer to NULL in corresponding
//	  MeasAxle.axleMeasEnts element and re-inits MeasEntity fields accordingly
//	- assignDefault() overwrites axleData and directionLow2Up fields, performing default
//	  axle alignment, changing MeasEntity isAssigned and axleIdx fields accordingly
//	  it uses reference timestamp vectors only if available for determining axle speed and
//	  train direction, otherwise it uses default values
//	  rcbPosTs and estAcc_mpss fields of the created axleData elements will be invalid
//	- processCurves() peforms curve fitting and evaluating operations for each assigned
//	  measurement Entity
//	- fitTheoretical() peforms curve fitting with local search algorithm
//	- evalCurve() calculates different parameters of measurement entity and determines if
//	  it can be used for load calculation
//	- setIsUsable() checks different parameters of the entities corresponding to the axle
//	  and sets the isUsable flag of the entities accordingly
//	- calcTemp() calculates temperature and set rcbAvgTemp and measAvgTemp fields based
//	  on the measEntityAll data after timestamp and curve processing
//	- scaleHeight() calculates the load for each measruement entity based on the fitted
//	  curve, axle speed, RCB temperature and RCB scale parameters
//	- calcAxleResults() calculates the following fields of AxleData objects: loadLeft_kg,
//	  loadRigth_kg, loadAx_kg, stdDevLoadAx_kg, numUsedSensPairs, worstE, worstWdE,
//	  wheelDiag
//	- calcSensorStats() calculate sensor status values based on previous results, and
//	  set members numRcbMeasAx, rcbStat and rsbStat
//	- doAveraging() averages the provided values, discarding the extremums if requested
//	  and there are at least the provided number of values available, and returns the
//	  (mean, standard deviation) pair as result
//	  discardIfAtLeast must be at least 3
//	- calcAxleWheelDiagVal() calculates the wheel diagnostic value of the axle
//
class MeasProcessor {
public:
	MeasProcessor(ConfigManager& confMgrInit, Logger& sysLogInit);
	void doProcessing();
	friend ostream& operator<<(ostream& os, const MeasProcessor& obj);
	void getSerializedResult(uint64_t timestamp, bool axleNumOk, vector<uint8_t>& serRes);

private:
	ConfigManager& confMgr;
	Logger& sysLog;

	//RCB measurement data
	vector<vector<MeasEntity> > measEntityAll;
	vector<vector<MeasEntity>* > measEntityLeft;
	vector<vector<MeasEntity>* > measEntityRight;
	vector<int> measPos_mm;

	//RSB reference timestamps and sensor positions
	vector<uint32_t> upperRefTs100us;
	vector<uint32_t> lowerRefTs100us;
	int upperPos_mm;
	int lowerPos_mm;

	//per-axle data
	vector<MeasAxle> axleData;
	bool directionLow2Up;

	//timestamp alignment flags
	bool alOkWoutTsRest;
	bool alOkWithTsRest;
	bool alUsingDef;

	//sensor-specific results
	vector<pair<bool, float> > rcbAvgTemp;	//<valid, avgTemp>
	float measAvgTemp;
	bool measAvgTempDefUsed;
	vector<unsigned int> numRsbMeasAx;
	vector<unsigned int> numRsbUnusableEntity;
	vector<unsigned int> numRcbMeasAx;
	vector<float> rsbStat;
	vector<float> rcbStat;

	void filterRsbEnts();
	void initMeasEnts(bool doLogging = false);
	void initRefTs();
	bool calcAxleTs();
	void alignMeasEnts();
	bool evalAlignment(bool evalCondHard);
	void restoreTs();
	void disassignErrEnts();
	void assignDefault();

	void processCurves();
	void fitTheoretical(MeasEntity& measEnt, const vector<int16_t>& currSamples, vector<int>& fittedCurve);
	void evalCurve(MeasEntity& measEnt, const vector<int16_t>& currSamples, const vector<int>& fittedCurve);
	void setIsUsable(MeasAxle& measAx);

	void calcTemps();
	void scaleHeights();
	void calcAxleResults();
	void calcSensorStats();
	static pair<float, float> doAveraging(const vector<float>& values, bool discExtremums, unsigned int discIfAtLeast);
	static float calcAxleWheelDiagVal(MeasAxle& measAx);
};

//class representing the optimizable function curve fitting error
//	- getScore() implements the virtual function and returns the error
//	  of the fitting based on the ideal curve parameters provided in
//	  paramVec, which must contain three element
//	- getters return the data corresponding to the last getScore() call
class CurveFittingError: public Optimizable {
public:
	CurveFittingError(const vector<int16_t>& convSamples, const ArithmeticVector& paramLowLimInit, const ArithmeticVector& paramHighLimInit);
	float getScore(const ArithmeticVector& paramVec);
	int getSumErr2() const;
	const vector<int>& getTransformedCurve() const;

private:
	vector<int16_t> samples;
	vector<int> transformedCurve;
	float sumErr2;
	const unsigned int numSamplesDiv2;
	const unsigned int numSamplesMinus1;
	const float deltaIdx;
	const unsigned int numSampTailIgnore;

	//reference curve parameters
	static const unsigned int idealCurveSize;
	static const unsigned int leftHalfLastId;
	static const unsigned int rightHalfFirstId;
	static const float idealCurve[1000];
};


}
#endif /* ERDM_PC_MEASPROCESSOR_H_ */
