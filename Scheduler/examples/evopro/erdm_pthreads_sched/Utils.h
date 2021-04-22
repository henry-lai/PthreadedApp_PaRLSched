/*
 * evopro Innovation Kft.
 *
 * sni
 *
 * File: utils.h
 * <Description>
 * Created on: Aug 7, 2014
 *
 * Author: pechan
 * (C) 2014. evopro Innovation Kft.
 *
 */

#ifndef ERDM_PC_UTILS_H_
#define ERDM_PC_UTILS_H_

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <map>
#include <tuple>

#include <sys/time.h>

#include <stdint.h>

using namespace std;

namespace sniutils {

//Exception class indicating OS failure
class OsExc: public runtime_error {
public:
	OsExc(const string& whatMsg) : runtime_error(whatMsg) {}
};

//Class for writing a character in hexa format to output stream
class CharAsHex {
public:
	CharAsHex(char cInit) :
		c(cInit) {
	}
	friend ostream& operator<<(ostream& stream, const CharAsHex& obj) {
		stream << hex << uppercase << "0x" << setw(2) << setfill('0') << ((int (obj.c)) & 0x000000FF) << setfill(' ') << setw(0) << nouppercase << dec;
		return stream;
	}
private:
	char c;
};

//Static class for general utility functions
//	- getTimeSec() returns current relative time in sec (OS monotonic clock)
//	  throws OsExc in case of failure
//	- getIntByte() returns a byte with index byteIdx from multiByteData integer
//	  index 0 is LSB
//	- concatIntBytes returns an integer created by concatenating the provided bytes
//	  byte0 is LSB
//	- sleepSec() sleeps until the specified timeout or until a signal interrupts execution
//	  returns immediately if the specified time is zero
//	- addSerialized() append the bytes of the provided variable to the vector in MSB order
//	- float2ushort() converts a float to unsigned short and set boundary value if necessary
class Utils {
public:
	static double getTimeSec();
	static void sleepSec(unsigned int numSec);

	template <typename IntType>
	static char getIntByte(IntType multiByteData, char byteIdx) {
		return (char (multiByteData >> (8*byteIdx)));
	}

	template <typename IntType>
	static IntType concatIntBytes(char byte0, char byte1) {

		IntType newInt;

		newInt = (IntType (byte0)) & 0x00FF;
		newInt |= ((IntType (byte1)) << 8) & 0xFF00;

		return newInt;
	}

	template <typename IntType>
	static IntType concatIntBytes(char byte0, char byte1, char byte2, char byte3) {

		IntType newInt;

		newInt = (IntType (byte0)) & 0x000000FF;
		newInt |= ((IntType (byte1)) << 8) & 0x0000FF00;
		newInt |= ((IntType (byte2)) << 16) & 0x00FF0000;
		newInt |= ((IntType (byte3)) << 24) & 0xFF000000;

		return newInt;
	}

	static void addSerialized(uint64_t value, vector<uint8_t>& serRes);
	static void addSerialized(uint32_t value, vector<uint8_t>& serRes);
	static void addSerialized(uint16_t value, vector<uint8_t>& serRes);
	static void addSerialized(float value, vector<uint8_t>& serRes);
	static uint16_t float2ushort(float value);

	static std::string lwHexEncode(unsigned char v);

private:
	Utils();
};

class Benchmark {
private:
	static std::vector<std::tuple<std::string, long, int> > values;
	static unsigned level;
	static long getMicros();
	long t0;
	std::string name;
public:
	Benchmark(const std::string& name);
	void start();
	void stop();
	static std::string getResults();
	static void writeResults(const std::string& filename);
};

} /* namespace sniutils */


#endif /* ERDM_PC_UTILS_H_ */
