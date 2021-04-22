/*
 * evopro Innovation Kft.
 *
 * sni
 *
 * File: SerialProtocol.cpp
 * <Description>
 * Created on: Aug 1, 2014
 *
 * Author: pechan
 * (C) 2014. evopro Innovation Kft.
 *
 */

#include "SerialProtocol.h"
#include "Utils.h"

#include <stdexcept>

using namespace sniutils;

namespace sniprotocol {

const char SerialProtocol::startChar = 0x2A;
const char SerialProtocol::stopChar = 0x2B;
const char SerialProtocol::escChar = 0x2C;
const char SerialProtocol::baseChar = 0x20;
const char SerialProtocol::gwAddr = 0x00;
const char SerialProtocol::broadcastAddr = 0xFF;
const unsigned int SerialProtocol::maxPayloadSize = 500;

//////////////////////////////////////////////////////////
//		function definitions for class OutMsgType
//////////////////////////////////////////////////////////

OutMsgType::OutMsgType(char msgCodeInit, char msgAnswCodeInit, unsigned int msgAnswLengthInit, bool nackAnswPossibleInit, bool lengthWord16Init, const char* msgNameInit) :
	msgCode(msgCodeInit),
	msgAnswCode(msgAnswCodeInit),
	msgAnswLength(msgAnswLengthInit),
	nackAnswPossible(nackAnswPossibleInit),
	lengthWord16(lengthWord16Init),
	msgName(msgNameInit) {
}

OutMsgType::~OutMsgType() {

}

char OutMsgType::getMsgAnswCode() const {
	return msgAnswCode;
}

unsigned int OutMsgType::getMsgAnswLength() const {
	return msgAnswLength;
}

char OutMsgType::getMsgCode() const {
	return msgCode;
}

bool OutMsgType::isNackAnswPossible() const {
	return nackAnswPossible;
}

bool OutMsgType::isLengthWord16() const {
	return lengthWord16;
}

const string& OutMsgType::getMsgName() const {
	return msgName;
}

//////////////////////////////////////////////////////////
//		function definitions for class SerialPacket
//////////////////////////////////////////////////////////

SerialPacket::SerialPacket() :
	destAddr(0),
	srcAddr(0),
	msgCode(0),
	appDataLength(0) {

}

SerialPacket::~SerialPacket() {

}

const vector<char>& SerialPacket::getAppData() const {
	return appData;
}

const vector<char>& SerialPacket::getRawData() const {
	return rawData;
}

char SerialPacket::getDestAddr() const {
	return destAddr;
}

char SerialPacket::getMsgCode() const {
	return msgCode;
}

char SerialPacket::getSrcAddr() const {
	return srcAddr;
}

unsigned int SerialPacket::calcCheckSum() const {

	unsigned int checkSum;
	unsigned int byteIdx;

	//checksum is the sum of the non-escaped message (without start and stop chars),
	//that is, destAddr + srcAddr + msgCode + appDataLength + bytes of appData
	//in case of short message, appDataLength = 0 and appData is empty
	checkSum = 0;
	checkSum += ((unsigned int) (destAddr)) & 0x000000FF;
	checkSum += ((unsigned int) (srcAddr)) & 0x000000FF;
	checkSum += ((unsigned int) (msgCode)) & 0x000000FF;
	checkSum += appDataLength;
	for (byteIdx = 0; byteIdx < appData.size(); byteIdx++) {
		checkSum += ((unsigned int) (appData[byteIdx])) & 0x000000FF;
	}

	return checkSum;
}

//////////////////////////////////////////////////////////
//		function definitions for class SerialPacketIn
//////////////////////////////////////////////////////////

SerialPacketIn::SerialPacketIn() :
		pktFinished(false),
		headErr(false),
		lengthErr(false),
		checkSumErr(false),
		msgState(SPSTATE_START),
		prevEscape(false),
		checkSumBytes(0),
		lengthWord16(false) {

}

SerialPacketIn::~SerialPacketIn() {

}

bool SerialPacketIn::addRawData(char newByte) {

	uint16_t lengthCand;

	//This function implements the serial protocol on the receiver side
	//First the special characters are handled (start, stop and escape),
	//then the new byte is treated according to the current state
	//State is stored in the member variable msgState and it always indicates
	//the expected meaning of the next byte

	//ignore character and return if packet is already finished
	if (pktFinished == true) {
		return false;
	}

	//handle packet start character
	if (newByte == SerialProtocol::startChar) {

		//if it was expected, add it and change state
		if (msgState == SPSTATE_START) {
			rawData.push_back(newByte);
			msgState = SPSTATE_DESTADDR;
			return true;
		}
		else {
			//otherwise do not add but set message as finished broken message
			pktFinished = true;
			lengthErr = true;
			return false;
		}
	}

	//if a start character should have been received set head error and
	//return since byte was processed (discarding it this way)
	if (msgState == SPSTATE_START) {
		headErr = true;
		return true;
	}

	//handle packet stop character
	if (newByte == SerialProtocol::stopChar) {

		//since a start character was received earlier (state is not SPSTATE_START),
		//this stop character stops the message - add it and set flag
		rawData.push_back(newByte);
		pktFinished = true;

		//but if it was unexpected, error flag should be set
		if (msgState != SPSTATE_STOPCAND) {
			lengthErr = true;
			return true;
		}

		//checksum must be the last two bytes stored in appData
		//move them to checksum, but check length first
		if (appData.size() < 2) {
			lengthErr = true;
			return true;
		}
		checkSumBytes = Utils::concatIntBytes<unsigned int>(appData[appData.size()-1], appData[appData.size()-2], 0, 0);
		appData.erase(appData.end()-2, appData.end());

		//if a message is a long message (there are length/payload bytes), process length field
		//either there is a one or a two byte length field - check if any version is consistent
		//(it can be proven that they cannot be consistent at the same time)
		if (appData.size() > 0) {

			//check if single-byte length is consistent
			if (appData.size() >= 1) {
				lengthCand = (uint16_t ((unsigned char) (appData[0])));
				if (lengthCand == appData.size() - 1) {

					//if single byte length field is detected, remove it from appdata,
					//do not set any error flags but set length word size flag
					appData.erase(appData.begin());
					lengthWord16 = false;
				}
				else {
					//if single-byte length is not valid, check the other option if possible
					if (appData.size() >= 2) {
						lengthCand = Utils::concatIntBytes<uint16_t>(appData[1], appData[0]);
						if (lengthCand == appData.size() - 2) {

							//if a double-byte length field is detected, remove bytes from appData,
							//also set corresponding flag
							appData.erase(appData.begin(), appData.begin()+2);
							lengthWord16 = true;
						}
						else {
							lengthErr = true;
							return true;
						}
					}
					else {
						lengthErr = true;
						return true;
					}
				}
			}
			else {
				lengthErr = true;
				return true;
			}
		}

		//length field was consistent (or it is a short message)
		//set appDataLength field now since appData stores the real payload bytes now
		appDataLength = appData.size();

		// now calculate checksum
		if (checkSumBytes != calcCheckSum()) {
			checkSumErr = true;
		}

		return true;
	}

	//if the new byte is neither a start, nor a stop char,
	//it should be part of this message - adding it to buffer, then handling it according
	//to the current state
	rawData.push_back(newByte);

	//handle escape character
	if (newByte == SerialProtocol::escChar) {
		prevEscape = true;
		return true;
	}

	//decode byte if previous one was an escape (and this one is not a START/STOP/ESC)
	if (prevEscape == true) {
		newByte = newByte ^ SerialProtocol::baseChar;
		prevEscape = false;
	}

	//save destination address
	if (msgState == SPSTATE_DESTADDR) {

		destAddr = newByte;
		msgState = SPSTATE_SRCADDR;
		return true;
	}

	//save source address
	if (msgState == SPSTATE_SRCADDR) {

		srcAddr = newByte;
		msgState = SPSTATE_MSGCODE;
		return true;
	}

	//save message code
	if (msgState == SPSTATE_MSGCODE) {

		msgCode = newByte;
		msgState = SPSTATE_STOPCAND;
		return true;
	}

	//handle state SPSTATE_STOPCAND
	//check if appdata does not exceed limit (payload limit + 2 length + 2 chsum bytes)
	//if it does, finishing message with length error, and not adding byte to this message
	//(since it is not a start char, it will cause a head error in the next message)
	if (appData.size() >= SerialProtocol::maxPayloadSize + 4) {
		pktFinished = true;
		lengthErr = true;
		return true;
	}

	//this byte may be a length, a payload or a checksum byte but this will be
	//determined after the stop char has been received - save byte as appData just now
	appData.push_back(newByte);
	return true;
}

void SerialPacketIn::addRawData(vector<char>& newBytes) {

	unsigned int byteIdx;

	//add bytes
	for (byteIdx = 0; byteIdx < newBytes.size(); byteIdx++) {

		//try to add byte, if not possible, exit from cycle
		if (addRawData(newBytes[byteIdx]) == false) {
			break;
		}
	}

	//remove bytes added from input vector
	if (byteIdx > 0) {
		newBytes.erase(newBytes.begin(), newBytes.begin()+byteIdx);
	}
}

bool SerialPacketIn::isPktOK() const {

	if ((pktFinished == true) && (lengthErr == false) && (checkSumErr == false)) {
		return true;
	}
	else {
		return false;
	}
}

bool SerialPacketIn::isPktFinished() const {
	return pktFinished;
}

bool SerialPacketIn::isHeadErr() const {
	return headErr;
}

bool SerialPacketIn::isLengthErr() const {
	return lengthErr;
}

bool SerialPacketIn::isCheckSumErr() const {
	return checkSumErr;
}

bool SerialPacketIn::isLengthWord16() const {
	return lengthWord16;
}

//////////////////////////////////////////////////////////
//		function definitions for class SerialPacketOut
//////////////////////////////////////////////////////////

SerialPacketOut::SerialPacketOut(const OutMsgType& msgTypeInit, char destAddrInit, const vector<char>& appDataInit, char srcAddrInit) :
	msgType(msgTypeInit) {

	unsigned int byteIdx;
	unsigned int checkSum;

	//set provided member values
	appData = appDataInit;
	destAddr = destAddrInit;
	srcAddr = srcAddrInit;
	msgCode = msgType.getMsgCode();

	//set app data length
	appDataLength = appDataInit.size();

	//check if length is below limit
	if (((appDataLength >= 256) && (msgTypeInit.isLengthWord16() == false)) || (appDataLength >= 65536)) {
		throw logic_error("Error constructing serial packet, length value too wide");
	}
	if (appDataLength > SerialProtocol::maxPayloadSize) {
		throw logic_error("Error constructing serial packet, length exceeds limit");
	}

	//construct packet: <start> <dest> <src> <msgcode> [<dlength upper> [<dlength_lower>] <appdata>] <chsum upper> <chsum lower> <stop>
	//add header
	rawData.push_back(SerialProtocol::startChar);
	addByteWithEscaping(destAddr);
	addByteWithEscaping(srcAddr);
	addByteWithEscaping(msgCode);

	//add length and app data bytes if any exists with respect to short/long message version
	//add length bytes in case of long messages even if no payload exists
	if ((appDataLength > 0) || (msgTypeInit.isLengthWord16() == true)) {
		if (msgTypeInit.isLengthWord16() == false) {
			addByteWithEscaping(Utils::getIntByte<unsigned int>(appDataLength, 0));
		}
		else {
			addByteWithEscaping(Utils::getIntByte<unsigned int>(appDataLength, 1));
			addByteWithEscaping(Utils::getIntByte<unsigned int>(appDataLength, 0));
		}
		for (byteIdx = 0; byteIdx < appDataLength; byteIdx++) {
			addByteWithEscaping(appData[byteIdx]);
		}
	}

	//calculate and add checksum bytes
	checkSum = calcCheckSum();
	addByteWithEscaping(Utils::getIntByte<unsigned int>(checkSum, 1));
	addByteWithEscaping(Utils::getIntByte<unsigned int>(checkSum, 0));

	//add stop character
	rawData.push_back(SerialProtocol::stopChar);

}

SerialPacketOut::~SerialPacketOut() {

}

void SerialPacketOut::addByteWithEscaping(char newByte) {

	//append escape character and calcualte escaped byte if needed
	if ((newByte == SerialProtocol::startChar) || (newByte == SerialProtocol::stopChar) || (newByte == SerialProtocol::escChar)) {
		rawData.push_back(SerialProtocol::escChar);
		newByte = newByte ^ SerialProtocol::baseChar;
	}

	//append data byte
	rawData.push_back(newByte);
}

//////////////////////////////////////////////////////////
//		function definitions for app data decoding classes
//////////////////////////////////////////////////////////

DevStatAnswData::DevStatAnswData() {

}

DevStatAnswData::DevStatAnswData(const SerialPacketIn& packetIn) {
	decodeData(packetIn);
}

void DevStatAnswData::decodeData(const SerialPacketIn& packetIn) {

	const vector<char>& appData = packetIn.getAppData();

	//check app data length, throw exception if not appropriate
	if (appData.size() != OutMsgType::msgDevStatRq().getMsgAnswLength()) {
		throw runtime_error("Error: length mismatch when creating DevStatAnswData");
	}

	moduleRcb = Utils::concatIntBytes<uint16_t>(appData[1], appData[0]);
	statFlashBoot = Utils::concatIntBytes<uint16_t>(appData[3], appData[2]);
	statFlashApp = Utils::concatIntBytes<uint16_t>(appData[5], appData[4]);
	rxbStat = Utils::concatIntBytes<uint16_t>(appData[7], appData[6]);
	rcbStat = Utils::concatIntBytes<uint16_t>(appData[9], appData[8]);
	autoZero = Utils::concatIntBytes<uint16_t>(appData[11], appData[10]);
}

bool DevStatAnswData::isRcb() const {

	if (moduleRcb == 0x4342) {
		return true;
	}
	else {
		return false;
	}
}

bool DevStatAnswData::isAutoZeroOk() const {

	if ((autoZero == 0x00AA) || (autoZero == 0x0002)) {
		return true;
	}
	else {
		return false;
	}
}

uint16_t DevStatAnswData::getAutoZero() const {
	return autoZero;
}

uint16_t DevStatAnswData::getModuleRcb() const {
	return moduleRcb;
}

uint16_t DevStatAnswData::getRcbStat() const {
	return rcbStat;
}

uint16_t DevStatAnswData::getRxbStat() const {
	return rxbStat;
}

uint16_t DevStatAnswData::getStatFlashApp() const {
	return statFlashApp;
}

uint16_t DevStatAnswData::getStatFlashBoot() const {
	return statFlashBoot;
}

bool DevStatAnswData::checkStat() const {
	return isAutoZeroOk();
}

RsbFastStatAnswData::RsbFastStatAnswData() {

}

RsbFastStatAnswData::RsbFastStatAnswData(const SerialPacketIn& packetIn) {
	decodeData(packetIn);
}

void RsbFastStatAnswData::decodeData(const SerialPacketIn& packetIn) {

	const vector<char>& appData = packetIn.getAppData();

	//check app data length, throw exception if not appropriate
	if (appData.size() != OutMsgType::msgRsbFastStatRq().getMsgAnswLength()) {
		throw runtime_error("Error: length mismatch when creating RsbFastStatAnswData");
	}

	axleCounts[0] = Utils::concatIntBytes<uint16_t>(appData[1], appData[0]);
	axleCounts[1] = Utils::concatIntBytes<uint16_t>(appData[3], appData[2]);
	status = Utils::concatIntBytes<uint16_t>(appData[5], appData[4]);
}

bool RsbFastStatAnswData::isArmed() const {

	if ((status & 0x0001) != 0) {
		return true;
	}
	else {
		return false;
	}
}

bool RsbFastStatAnswData::isMeasInProgress() const {

	if ((status & 0x0002) != 0) {
		return true;
	}
	else {
		return false;
	}
}

uint16_t RsbFastStatAnswData::getAxleCount(unsigned int dataIdx) const {

	if (dataIdx > 1) {
		dataIdx = 1;
	}
	return axleCounts[dataIdx];
}

uint16_t RsbFastStatAnswData::getStatus() const {
	return status;
}

bool RsbFastStatAnswData::checkStat() const {
	return isArmed();
}

RcbFastStatAnswData::RcbFastStatAnswData() {

}

RcbFastStatAnswData::RcbFastStatAnswData(const SerialPacketIn& packetIn) {
	decodeData(packetIn);
}

void RcbFastStatAnswData::decodeData(const SerialPacketIn& packetIn) {

	const vector<char>& appData = packetIn.getAppData();

	//check app data length, throw exception if not appropriate
	if (appData.size() != OutMsgType::msgRcbFastStatRq().getMsgAnswLength()) {
		throw runtime_error("Error: length mismatch when creating RcbFastStatAnswData");
	}

	axleCount = Utils::concatIntBytes<uint16_t>(appData[1], appData[0]);
	status = Utils::concatIntBytes<uint16_t>(appData[3], appData[2]);
}

bool RcbFastStatAnswData::isArmed() const {

	if ((status & 0x0001) != 0) {
		return true;
	}
	else {
		return false;
	}
}

uint16_t RcbFastStatAnswData::getAxleCount() const {
	return axleCount;
}

uint16_t RcbFastStatAnswData::getStatus() const {
	return status;
}

bool RcbFastStatAnswData::checkStat() const {
	return isArmed();
}

AxleCntAnswData::AxleCntAnswData(const SerialPacketIn& packetIn) {

	const vector<char>& appData = packetIn.getAppData();

	//check app data length, throw exception if not appropriate
	if (appData.size() != OutMsgType::msgAxleCntRq().getMsgAnswLength()) {
		throw runtime_error("Error: length mismatch when creating AxleCntAnswData");
	}

	axleCount = Utils::concatIntBytes<uint16_t>(appData[1], appData[0]);

}

uint16_t AxleCntAnswData::getAxleCount() const {
	return axleCount;
}

MeasLdataAnswData::MeasLdataAnswData() {
	dummy = true;
}

bool MeasLdataAnswData::isDummy() const {
	return dummy;
}

RsbMeasLdataAnswData::RsbMeasLdataAnswData() {

}

RsbMeasLdataAnswData::RsbMeasLdataAnswData(const SerialPacketIn& packetIn) {
	decodeData(packetIn);
}

void RsbMeasLdataAnswData::decodeData(const SerialPacketIn& packetIn) {

	const vector<char>& appData = packetIn.getAppData();
	unsigned int sensIdx;

	//check app data length, throw exception if not appropriate
	if (appData.size() != OutMsgType::msgRsbMeasLdataRq().getMsgAnswLength()) {
		throw runtime_error("Error: length mismatch when creating RsbMeasLdataAnswData");
	}

	axleId = Utils::concatIntBytes<uint16_t>(appData[1], appData[0]);
	for (sensIdx = 0; sensIdx < 2; sensIdx++) {
		status[sensIdx] = Utils::concatIntBytes<uint16_t>(appData[12*sensIdx+3], appData[12*sensIdx+2]);
		pulseLength100us[sensIdx] = Utils::concatIntBytes<uint16_t>(appData[12*sensIdx+5], appData[12*sensIdx+4]);
		timeStamp100us[sensIdx] = Utils::concatIntBytes<uint32_t>(appData[12*sensIdx+9], appData[12*sensIdx+8], appData[12*sensIdx+7], appData[12*sensIdx+6]);
		temp[sensIdx] = Utils::concatIntBytes<int16_t>(appData[12*sensIdx+11], appData[12*sensIdx+10]);
		usable[sensIdx] = true;
	}

	dummy = false;

}

bool RsbMeasLdataAnswData::isValid(unsigned int sensIdx) const {

	normalizeIdx(&sensIdx);

	if ((status[sensIdx] & 0x0001) == 0) {
		return true;
	}
	else {
		return false;
	}
}

void RsbMeasLdataAnswData::setAsUnusable(unsigned int sensIdx) {

	normalizeIdx(&sensIdx);
	usable[sensIdx] = false;
}

bool RsbMeasLdataAnswData::isUsable(unsigned int sensIdx) const {

	normalizeIdx(&sensIdx);
	return usable[sensIdx];
}

uint16_t RsbMeasLdataAnswData::getAxleId() const {
	return axleId;
}

uint16_t RsbMeasLdataAnswData::getPulseLength100us(unsigned int sensIdx) const {
	normalizeIdx(&sensIdx);
	return pulseLength100us[sensIdx];
}

uint16_t RsbMeasLdataAnswData::getStatus(unsigned int sensIdx) const {
	normalizeIdx(&sensIdx);
	return status[sensIdx];
}

uint32_t RsbMeasLdataAnswData::getTimeStamp100us(unsigned int sensIdx) const {
	normalizeIdx(&sensIdx);
	return timeStamp100us[sensIdx];
}

int16_t RsbMeasLdataAnswData::getTemp(unsigned int sensIdx) const {
	normalizeIdx(&sensIdx);
	return temp[sensIdx];
}

bool RsbMeasLdataAnswData::isValid() const {
	return isValid(0) | isValid(1);
}

json::Object RsbMeasLdataAnswData::toJsonObj() const {

	unsigned int elementIdx;

	//create and fill json arrays
	json::Array statusData;
	json::Array pulseLengthData;
	json::Array timeStampData;
	json::Array tempData;
	for (elementIdx = 0; elementIdx < 2; elementIdx++) {
		statusData.push_back(status[elementIdx]);
		pulseLengthData.push_back(pulseLength100us[elementIdx]);
		timeStampData.push_back(int (timeStamp100us[elementIdx]));
		tempData.push_back(temp[elementIdx]);
	}

	//create and fill measurement entry json object
	json::Object measEntry;
	measEntry["Ax"] = axleId;
	measEntry["Stat"] = statusData;
	measEntry["T"] = tempData;
	measEntry["Pl"] = pulseLengthData;
	measEntry["Ts"] = timeStampData;

	return measEntry;
}

void RsbMeasLdataAnswData::fromJsonObj(const json::Object& jsonObj) {

	unsigned int entryIdx;

	json::Array statusData = getArrayJsonField(jsonObj, "Stat");
	json::Array pulseLengthData = getArrayJsonField(jsonObj, "Pl");
	json::Array timeStampData = getArrayJsonField(jsonObj, "Ts");
	json::Array tempData = getArrayJsonField(jsonObj, "T");

	for (entryIdx = 0; entryIdx < 2; entryIdx++) {
		status[entryIdx] = uint16_t (statusData[entryIdx].ToInt());
		pulseLength100us[entryIdx] = uint16_t (pulseLengthData[entryIdx].ToInt());
		timeStamp100us[entryIdx] = uint32_t (timeStampData[entryIdx].ToInt());
		tempData[entryIdx] = int16_t (tempData[entryIdx].ToInt());
		usable[entryIdx] = true;
	}

	axleId = getIntJsonField(jsonObj, "Ax");

	dummy = false;
}

void RsbMeasLdataAnswData::normalizeIdx(unsigned int* sensIdx) {

	if (*sensIdx > 1) {
		*sensIdx = 1;
	}
	return;
}

RcbMeasLdataAnswData::RcbMeasLdataAnswData(bool numSamp64Init) :
	numSamp64(numSamp64Init) {

}

RcbMeasLdataAnswData::RcbMeasLdataAnswData(const SerialPacketIn& packetIn, bool numSamp64Init) :
	numSamp64(numSamp64Init) {
	decodeData(packetIn);
}

void RcbMeasLdataAnswData::decodeData(const SerialPacketIn& packetIn) {

	const vector<char>& appData = packetIn.getAppData();
	unsigned int sampleIdx;

	//check app data length, throw exception if not appropriate
	if (appData.size() != OutMsgType::msgRcbMeasLdataRq(numSamp64).getMsgAnswLength()) {
		throw runtime_error("Error: length mismatch when creating RsbMeasLdataAnswData");
	}

	axleId = Utils::concatIntBytes<uint16_t>(appData[1], appData[0]);
	status = Utils::concatIntBytes<uint16_t>(appData[3], appData[2]);
	offset = Utils::concatIntBytes<int16_t>(appData[5], appData[4]);
	pulseLength100us = Utils::concatIntBytes<uint16_t>(appData[7], appData[6]);
	timeStamp100us = Utils::concatIntBytes<uint32_t>(appData[11], appData[10], appData[9], appData[8]);
	temp = Utils::concatIntBytes<int16_t>(appData[13], appData[12]);
	dummy = false;

	//add samples to vector
	samples.clear();
	for (sampleIdx = 14; sampleIdx < OutMsgType::msgRcbMeasLdataRq(numSamp64).getMsgAnswLength(); sampleIdx+=2) {
		samples.push_back(Utils::concatIntBytes<int16_t>(appData[sampleIdx+1], appData[sampleIdx]));
	}

}

vector<int16_t>& RcbMeasLdataAnswData::getSamples() {
	return samples;
}

int16_t RcbMeasLdataAnswData::getOffset() const {
	return offset;
}

bool RcbMeasLdataAnswData::isValid() const {

	//TODO: 0xEEEE can be removed if this condition is indicated in bit3 of status
	if (((status & 0x0007) != 0) || ((pulseLength100us == 0xEEEE) && (timeStamp100us == 0xEEEE))) {
		return false;
	}
	else {
		return true;
	}
}

uint16_t RcbMeasLdataAnswData::getAxleId() const {
	return axleId;
}

uint16_t RcbMeasLdataAnswData::getPulseLength100us() const {
	return pulseLength100us;
}

uint16_t RcbMeasLdataAnswData::getStatus() const {
	return status;
}

uint32_t RcbMeasLdataAnswData::getTimeStamp100us() const {
	return timeStamp100us;
}

int16_t RcbMeasLdataAnswData::getTemp() const {
	return temp;
}

float RcbMeasLdataAnswData::getTempC() const {
	return (float (temp))/10.0f;
}

json::Object RcbMeasLdataAnswData::toJsonObj() const {

	unsigned int sampleIdx;

	//create and fill json array for measurement samples
	json::Array samplesJson;
	for (sampleIdx = 0; sampleIdx < samples.size(); sampleIdx++) {
		samplesJson.push_back(samples[sampleIdx]);
	}

	//create and fill measurement entry json object
	json::Object measEntry;
	measEntry["Ax"] = axleId;
	measEntry["Stat"] = status;
	measEntry["Off"] = offset;
	measEntry["T"] = temp;
	measEntry["Pl"] = pulseLength100us;
	measEntry["Ts"] = int (timeStamp100us);
	measEntry["Samp"] = samplesJson;

	return measEntry;
}

void RcbMeasLdataAnswData::fromJsonObj(const json::Object& jsonObj) {

	unsigned int sampIdx;

	axleId = getIntJsonField(jsonObj, "Ax");
	status = getIntJsonField(jsonObj, "Stat");
	offset = getIntJsonField(jsonObj, "Off");
	temp = getIntJsonField(jsonObj, "T");
	pulseLength100us = getIntJsonField(jsonObj, "Pl");
	timeStamp100us = getIntJsonField(jsonObj, "Ts");

	samples.clear();
	json::Array samplesJson = getArrayJsonField(jsonObj, "Samp");
	for (sampIdx = 0; sampIdx < samplesJson.size(); sampIdx++) {
		samples.push_back(int16_t (int (samplesJson[sampIdx])));
	}

	if (samples.size() == 64) {
		numSamp64 = true;
	}
	else {
		numSamp64 = false;
	}

	dummy = false;
}

VerRqAnswData::VerRqAnswData(const SerialPacketIn& packetIn) {

	const vector<char>& appData = packetIn.getAppData();

	//check app data length, throw exception if not appropriate
	if (appData.size() != OutMsgType::msgVerRq().getMsgAnswLength()) {
		throw runtime_error("Error: length mismatch when creating VerRqAnswData");
	}

	serialNumber = Utils::concatIntBytes<uint32_t>(appData[39], appData[38], appData[37], appData[36]);

}

uint32_t VerRqAnswData::getSerialNumber() const {
	return serialNumber;
}

FlashBusyCheckData::FlashBusyCheckData(const SerialPacketIn& packetIn) {

	const vector<char>& appData = packetIn.getAppData();

	//check app data length, throw exception if not appropriate
	if (appData.size() != OutMsgType::msgFlashBusyCheck().getMsgAnswLength()) {
		throw runtime_error("Error: length mismatch when creating FlashBusyCheckData");
	}

	busy = false;
	if (Utils::concatIntBytes<uint16_t>(appData[1], appData[0]) != 0) {
		busy = true;
	}
}

bool FlashBusyCheckData::isBusy() const {
	return busy;
}

ReadFwData::ReadFwData(const SerialPacketIn& packetIn) {

	const vector<char>& appData = packetIn.getAppData();

	//check app data length, throw exception if not appropriate
	//instead of expected length (which is unknown), compare to minimal sensible legnth
	if (appData.size() < 7) {
		throw runtime_error("Error: length too low when creating ReadFwData");
	}

	address = Utils::concatIntBytes<uint32_t>(appData[3], appData[2], appData[1], appData[0]);

	//get length field and check that it equals the number of remaining data bytes
	uint16_t length = Utils::concatIntBytes<uint16_t>(appData[5], appData[4]);
	if (length != appData.size() - 6) {
		throw runtime_error("Error: length mismatch when creating ReadFwData");
	}

	//copy data bytes
	data.insert(data.begin(), appData.begin()+6, appData.end());
}

uint32_t ReadFwData::getAddress() {
	return address;
}

const vector<char>& ReadFwData::getData() {
	return data;
}

ClearSectorCmdData::ClearSectorCmdData(uint32_t address) {

	appData.push_back(Utils::getIntByte<uint32_t>(address, 3));
	appData.push_back(Utils::getIntByte<uint32_t>(address, 2));
	appData.push_back(Utils::getIntByte<uint32_t>(address, 1));
	appData.push_back(Utils::getIntByte<uint32_t>(address, 0));
}

const vector<char>& ClearSectorCmdData::getAppData() {
	return appData;
}

ReadFwCmdData::ReadFwCmdData(uint32_t address, uint16_t length) : ClearSectorCmdData(address) {

	//append length
	appData.push_back(Utils::getIntByte<uint16_t>(length, 1));
	appData.push_back(Utils::getIntByte<uint16_t>(length, 0));
}

ProgFwCmdData::ProgFwCmdData(uint32_t address, const vector<char>& fwData) : ReadFwCmdData(address, (uint16_t) fwData.size()) {

	if ((fwData.size() == 0) || (fwData.size() > 65535)) {
		throw runtime_error("Error creating Program Firmware AppData: data length out of range");
	}

	//copy firmware data
	appData.insert(appData.end(), fwData.begin(), fwData.end());
}

} /* namespace sniprotocol */

