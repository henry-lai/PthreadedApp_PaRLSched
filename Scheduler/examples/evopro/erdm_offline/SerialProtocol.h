/*
 * evopro Innovation Kft.
 *
 * sni
 *
 * File: SerialProtocol.h
 * <Description>
 * Created on: Aug 1, 2014
 *
 * Author: pechan
 * (C) 2014. evopro Innovation Kft.
 *
 */

#ifndef ERDM_PC_SERIALPROTOCOL_H_
#define ERDM_PC_SERIALPROTOCOL_H_

#include "json.h"

#include <vector>
#include <string>
#include <stdint.h>

using namespace std;

namespace sniprotocol {

//typedef enum for incoming message states
typedef enum {
	SPSTATE_START,
	SPSTATE_DESTADDR,
	SPSTATE_SRCADDR,
	SPSTATE_MSGCODE,
	SPSTATE_STOPCAND
} spMsgState;

//Class for storing protocol-related constants
class SerialProtocol {
public:
	static const char startChar;
	static const char stopChar;
	static const char escChar;
	static const char baseChar;
	static const char gwAddr;
	static const char broadcastAddr;
	static const unsigned int maxPayloadSize;
private:
	SerialProtocol();
};

//Class for storing output message code and reply parameters
//Type objects can be acquired with static methods
class OutMsgType {
public:
	~OutMsgType();
	char getMsgAnswCode() const;
	unsigned int getMsgAnswLength() const;
	char getMsgCode() const;
	bool isNackAnswPossible() const;
	bool isLengthWord16() const;
	const string& getMsgName() const;

	//message type info for every kind of host->sensor messages
	static OutMsgType msgEchoRq() { return OutMsgType(0x11, 0x12, 0, false, false, "Echo Message"); }
	static OutMsgType msgResetRq() { return OutMsgType(0x85, 0x70, 0, false, false, "Reset Message"); }
	static OutMsgType msgDevStatRq() { return OutMsgType(0x80, 0x81, 16, false, false, "Status Request Message"); }
	static OutMsgType msgRcbFastStatRq() { return OutMsgType(0x78, 0x79, 4, false, false, "RCB Fast Status Request Message"); }
	static OutMsgType msgMonStatRq() { return OutMsgType(0x82, 0x83, 16, false, false, "Monitor Statistic Request Message"); }
	static OutMsgType msgTempRq() { return OutMsgType(0xB3, 0xB4, 2, false, false, "Temperature Request Message"); }
	static OutMsgType msgRcbStatMsrRq() { return OutMsgType(0xAD, 0xAE, 24, false, false, "RCB Static Measurement Message"); }
	static OutMsgType msgRsbStatMsrRq() { return OutMsgType(0xAD, 0xAE, 10, false, false, "RSB Static Measurement Message"); }
	static OutMsgType msgAutoZeroRq() { return OutMsgType(0xAA, 0x70, 0, false, false, "Autozero Request Message"); }
	static OutMsgType msgAutoZeroWztRq() { return OutMsgType(0xAC, 0x70, 0, false, false, "Forced Autozero Request Message"); }
	static OutMsgType msgRcbArmRq() { return OutMsgType(0xA0, 0x70, 0, true, false, "RCB Arm Request Message"); }
	static OutMsgType msgRsbArmRq() { return OutMsgType(0xF1, 0x70, 0, false, false, "RSB Arm Request Message"); }
	static OutMsgType msgDisarmRq() { return OutMsgType(0xB0, 0x70, 0, false, false, "Disarm Request Message"); }
	static OutMsgType msgAxleCntRq() { return OutMsgType(0xC0, 0xC1, 2, false, false, "Axle Count Request Message"); }
	static OutMsgType msgRsbFirstAxTime() { return OutMsgType(0xBA, 0xBB, 4, false, false, "First Axle Time Readout Message"); }
	static OutMsgType msgRcbMeasReadout32Rq() { return OutMsgType(0xC6, 0xC3, 78, false, false, "RCB Measurement Readout Request Message"); }
	static OutMsgType msgRcbMeasLdata32Rq() { return OutMsgType(0xC2, 0xC3, 78, false, false, "RCB Read Out One Measurement Message"); }
	static OutMsgType msgRcbMeasReadout64Rq() { return OutMsgType(0xC6, 0xC3, 142, false, false, "RCB Measurement Readout Request Message"); }
	static OutMsgType msgRcbMeasLdata64Rq() { return OutMsgType(0xC2, 0xC3, 142, false, false, "RCB Read Out One Measurement Message"); }
	static OutMsgType msgRcbMeasReadoutRq(bool numSamp64) { return (numSamp64 ? msgRcbMeasReadout64Rq() : msgRcbMeasReadout32Rq()); }
	static OutMsgType msgRcbMeasLdataRq(bool numSamp64) { return (numSamp64 ? msgRcbMeasLdata64Rq() : msgRcbMeasLdata32Rq()); }
	static OutMsgType msgRsbCtabReadRq() { return OutMsgType(0x44, 0x45, 36, false, false, "RSB Constant Table Read Message"); }
	static OutMsgType msgRcbCtabReadRq() { return OutMsgType(0x44, 0x45, 88, false, false, "RCB Constant Table Read Message"); }
	static OutMsgType msgCtabWriteRq() { return OutMsgType(0x47, 0x70, 0, true, false, "Constant Table Write Message"); }
	static OutMsgType msgCtabSaveRq() { return OutMsgType(0x49, 0x70, 0, true, false, "Save Constants to Flash Message"); }
	static OutMsgType msgCtabLoadDefRq() { return OutMsgType(0x48, 0x70, 0, false, false, "Load Default Constant Table Message"); }
	static OutMsgType msgVerRq() { return OutMsgType(0x55, 0x56, 96, false, false, "Version Message"); }
	static OutMsgType msgReadNumRecMeasStartRq() { return OutMsgType(0x57, 0x58, 2, false, false, "Read Number of Received Measurement Starts Message"); }
	static OutMsgType msgServMeasRq() { return OutMsgType(0xB5, 0x70, 0, false, false, "Start Service Measurement Message"); }
	static OutMsgType msgServBufReadRq() { return OutMsgType(0xB6, 0xB7, 42, false, false, "Read Service Buffer Data Message"); }
	static OutMsgType msgRsbFastStatRq() { return OutMsgType(0x78, 0x79, 6, false, false, "RSB Fast Status Request Message"); }
	static OutMsgType msgRsbMeasReadoutRq() { return OutMsgType(0xCC, 0xCB, 26, false, false, "RSB Measurement Readout Request Message"); }
	static OutMsgType msgRsbMeasLdataRq() { return OutMsgType(0xCA, 0xCB, 26, false, false, "RSB Read Out One Measurement Message"); }
	static OutMsgType msgClearSector() { return OutMsgType(0x7D, 0x70, 0, true, true, "Clear Sector Message"); }
	static OutMsgType msgStopApp() { return OutMsgType(0x71, 0x70, 0, false, true, "Stop Application Message"); }
	static OutMsgType msgFlashBusyCheck() { return OutMsgType(0x1E, 0x1E, 2, false, true, "Flash Busy Check Message"); }
	static OutMsgType msgProgFw() { return OutMsgType(0xAB, 0x70, 0, true, true, "Program Firmware Message"); }
	static OutMsgType msgReadFw(unsigned int queryNumBytes) { return OutMsgType(0xCC, 0xCC, queryNumBytes+6, true, true, "Read Firmware Message"); }

	//message type info for messages sent by sensors as master
	//these are not "out messages" (must not send them out), msgCode is thus invalid
	static OutMsgType inMsgMeasStartRq() { return OutMsgType(0xFF, 0xA1, 0, false, false, "Measurement Start Request Message"); }
	static OutMsgType inMsgMeasStopRq() { return OutMsgType(0xFF, 0xA2, 0, false, false, "Measurement Stop Request Message"); }

private:
	OutMsgType(char msgCodeInit, char msgAnswCodeInit, unsigned int msgAnswLengthInit, bool nackAnswPossibleInit, bool lengthWord16Init, const char* msgNameInit);
	char msgCode;
	char msgAnswCode;
	unsigned int msgAnswLength;
	bool nackAnswPossible;
	bool lengthWord16;
	string msgName;
};

//Class for defining common members of SerialPacketIn and SerialPacketOut
//Getter functions are available for accessing internal fields
class SerialPacket {
public:
	~SerialPacket();
	SerialPacket();
	const vector<char>& getAppData() const;
	const vector<char>& getRawData() const;
	char getDestAddr() const;
	char getMsgCode() const;
	char getSrcAddr() const;
protected:
	vector<char> rawData;
	char destAddr;
	char srcAddr;
	char msgCode;
	unsigned int appDataLength;
	vector<char> appData;
	unsigned int calcCheckSum() const;
};

//Class for creating packets from the bytes read from serial port
//	- addRawData(char) tries to add a new byte to the message and returns true if it was added (processed)
//	  the byte will not be added if it belongs to the next message (message is finished, second STX char, ect.)
//    true will be returned but the byte is discarded if STX was expected but a different character was added
//	- addRawData(vector<char>&) adds as many bytes from the beginning of the vector to the message as possible
//	  the processed bytes will be removed from the vector
//	  if the returned vector contains any bytes, they correspond to the next message
//  - isPktFinished() returns true if the message is finished
//  - isHeadErr() returns true, if invalid bytes were discarded before the start character was added
//	  (message may be still valid)
//  - isLengthErr() returns true, if a broken packet was received (start/stop received before expected, etc.)
//	- isCheckSumErr() returns true in case of a checksum mismatch (invalid in case of length error)
//  - isPktOK() returns true if the packet is finished, and there is neither length, nor checksum error
//	  in this case the decoded fields of the message are valid
//	- isLengthWord16() returns true if a double-byte length field was detected and returns false otherwise
class SerialPacketIn: public SerialPacket {
public:
	SerialPacketIn();
	~SerialPacketIn();
	bool addRawData(char newByte);
	void addRawData(vector<char>& newBytes);
	bool isPktOK() const;
	bool isPktFinished() const;
	bool isHeadErr() const;
	bool isLengthErr() const;
	bool isCheckSumErr() const;
	bool isLengthWord16() const;
private:
	bool pktFinished;
	bool headErr;
	bool lengthErr;
	bool checkSumErr;
	spMsgState msgState;
	bool prevEscape;
	unsigned int checkSumBytes;
	bool lengthWord16;
};

//Class for creating packets based on the specified message field values
class SerialPacketOut: public SerialPacket {
public:
	SerialPacketOut(const OutMsgType& msgTypeInit, char destAddrInit = SerialProtocol::broadcastAddr, const vector<char>& appDataInit = vector<char>(), char srcAddrInit = SerialProtocol::gwAddr);
	~SerialPacketOut();
	const OutMsgType& msgType;
private:
	void addByteWithEscaping(char newByte);
};

//virtual class for those incoming packets whose status can be OK or not OK
//	- decodeData() must be called first to decode incoming packet, may throw exception
//	  if length of packetIn is not appropriate (other fields are not checked)
//	- checkStat() returns true if status is OK, and false otherwise
class AnswDataCheckable {
public:
	AnswDataCheckable() {}
	virtual ~AnswDataCheckable() {}
	virtual void decodeData(const SerialPacketIn& packetIn) = 0;
	virtual bool checkStat() const = 0;
};

//classes for decoding incoming packet fields
//	- parametric constructors throw exception if length of packetIn data is not appropriate
//	  however, message type and other fields are not checked
//	- empty constructor creates empty object, decodeData() must be called before any other calls,
//	  otherwise return values are invalid

//	- ckeckStat() checks for autozero status
class DevStatAnswData : public AnswDataCheckable {
public:
	DevStatAnswData();
	DevStatAnswData(const SerialPacketIn& packetIn);
	void decodeData(const SerialPacketIn& packetIn);
	bool isRcb() const;
	bool isAutoZeroOk() const;
	uint16_t getAutoZero() const;
	uint16_t getModuleRcb() const;
	uint16_t getRcbStat() const;
	uint16_t getRxbStat() const;
	uint16_t getStatFlashApp() const;
	uint16_t getStatFlashBoot() const;
	bool checkStat() const;
private:
	uint16_t moduleRcb;
	uint16_t statFlashBoot;
	uint16_t statFlashApp;
	uint16_t rxbStat;
	uint16_t rcbStat;
	uint16_t autoZero;
};

//	- checkStat() checks for arm status
//	- getAxleCount() dataIdx = 0 refers to A1 value, = 1 to A2
class RsbFastStatAnswData : public AnswDataCheckable {
public:
	RsbFastStatAnswData();
	RsbFastStatAnswData(const SerialPacketIn& packetIn);
	void decodeData(const SerialPacketIn& packetIn);
	bool isArmed() const;
	bool isMeasInProgress() const;
	uint16_t getAxleCount(unsigned int dataIdx) const;
	uint16_t getStatus() const;
	bool checkStat() const;
private:
	uint16_t axleCounts[2];
	uint16_t status;
};

//	- checkStat() checks for arm status
class RcbFastStatAnswData : public AnswDataCheckable {
public:
	RcbFastStatAnswData();
	RcbFastStatAnswData(const SerialPacketIn& packetIn);
	void decodeData(const SerialPacketIn& packetIn);
	bool isArmed() const;
	uint16_t getAxleCount() const;
	uint16_t getStatus() const;
	bool checkStat() const;
private:
	uint16_t axleCount;
	uint16_t status;
};

class AxleCntAnswData {
public:
	AxleCntAnswData(const SerialPacketIn& packetIn);
	uint16_t getAxleCount() const;
private:
	uint16_t axleCount;
};

//class for common behavior of RSB/RCB Ldata answer storing dummy and valid state
//	- toJsonObj() returns the packet as a json object
//	  only gives valid result if the entry is not dummy and it is valid
class MeasLdataAnswData {
public:
	MeasLdataAnswData();
	virtual ~MeasLdataAnswData() {}
	bool isDummy() const;
	virtual bool isValid() const = 0;
	virtual json::Object toJsonObj() const = 0;

protected:
	bool dummy;
};

class RsbMeasLdataAnswData: public MeasLdataAnswData {
public:
	RsbMeasLdataAnswData();
	RsbMeasLdataAnswData(const SerialPacketIn& packetIn);
	void decodeData(const SerialPacketIn& packetIn);
	uint16_t getAxleId() const;
	uint16_t getPulseLength100us(unsigned int sensIdx) const;
	uint16_t getStatus(unsigned int sensIdx) const;
	uint32_t getTimeStamp100us(unsigned int sensIdx) const;
	int16_t getTemp(unsigned int sensIdx) const;
	bool isValid(unsigned int sensIdx) const;
	bool isValid() const;
	void setAsUnusable(unsigned int sensIdx);
	bool isUsable(unsigned int sensIdx) const;
	json::Object toJsonObj() const;
	void fromJsonObj(const json::Object& jsonObj);

private:
	uint16_t axleId;
	uint16_t status[2];
	uint16_t pulseLength100us[2];
	uint32_t timeStamp100us[2];
	int16_t temp[2];
	bool usable[2];
	static void normalizeIdx(unsigned int* sensIdx);
};

//constructor without packetIn creates empty object, decodeData must be called before use
class RcbMeasLdataAnswData: public MeasLdataAnswData {
public:
	RcbMeasLdataAnswData(bool numSamp64Init = false);
	RcbMeasLdataAnswData(const SerialPacketIn& packetIn, bool numSamp64Init);
	void decodeData(const SerialPacketIn& packetIn);
	vector<int16_t>& getSamples();
	int16_t getOffset() const;
	uint16_t getAxleId() const;
	uint16_t getPulseLength100us() const;
	uint16_t getStatus() const;
	uint32_t getTimeStamp100us() const;
	int16_t getTemp() const;
	float getTempC() const;
	bool isValid() const;
	json::Object toJsonObj() const;
	void fromJsonObj(const json::Object& jsonObj);

private:
	bool numSamp64;
	uint16_t axleId;
	uint16_t status;
	uint16_t pulseLength100us;
	uint32_t timeStamp100us;
	int16_t offset;
	int16_t temp;
	vector<int16_t> samples;
};

class VerRqAnswData {
public:
	VerRqAnswData(const SerialPacketIn& packetIn);
	uint32_t getSerialNumber() const;
private:
	uint32_t serialNumber;
};

class FlashBusyCheckData {
public:
	FlashBusyCheckData(const SerialPacketIn& packetIn);
	bool isBusy() const;
private:
	bool busy;
};

class ReadFwData {
public:
	ReadFwData(const SerialPacketIn& packetIn);
	uint32_t getAddress();
	const vector<char>& getData();
private:
	uint32_t address;
	vector<char> data;
};

//classes for creating appData of outgoing messages
//	- constructor must be supplied with specific message data
//	- getAppData() returns the packet application data

class ClearSectorCmdData {
public:
	ClearSectorCmdData(uint32_t address);
	const vector<char>& getAppData();
protected:
	vector<char> appData;
};

class ReadFwCmdData: public ClearSectorCmdData {
public:
	ReadFwCmdData(uint32_t address, uint16_t length);
};

//constructor throws exception if length of fwData is zero or higher than 65535
class ProgFwCmdData: public ReadFwCmdData {
public:
	ProgFwCmdData(uint32_t address, const vector<char>& fwData);
};

} /* namespace sniprotocol */
#endif /* ERDM_PC_SERIALPROTOCOL_H_ */
