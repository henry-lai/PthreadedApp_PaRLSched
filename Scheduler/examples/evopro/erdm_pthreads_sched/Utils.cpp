/*
 * evopro Innovation Kft.
 *
 * sni
 *
 * File: Utils.cpp
 * <Description>
 * Created on: Aug 7, 2014
 *
 * Author: pechan
 * (C) 2014. evopro Innovation Kft.
 *
 */

#include <cstring>

#include <errno.h>
#include <time.h>

#include <sstream>
#include <fstream>

#include "Utils.h"

namespace sniutils {

double Utils::getTimeSec() {

	struct timespec ts;
	int timeStat;
	int currErrno;

	timeStat = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (timeStat == -1) {
		currErrno = errno;
		throw OsExc(string("Error getting time: ").append(strerror(currErrno)));
	}

	return ts.tv_sec + ((double)ts.tv_nsec) / 1000000000.0;
}

void Utils::sleepSec(unsigned int numSec) {

	struct timespec ts;

	if (numSec == 0) {
		return;
	}

	ts.tv_sec = numSec;
	ts.tv_nsec = 0;

	//TODO: revise this method if any signals are used during program execution
	nanosleep(&ts, NULL);
}

void Utils::addSerialized(uint64_t value, vector<uint8_t>& serRes) {

	serRes.push_back(Utils::getIntByte<uint64_t>(value, 7));
	serRes.push_back(Utils::getIntByte<uint64_t>(value, 6));
	serRes.push_back(Utils::getIntByte<uint64_t>(value, 5));
	serRes.push_back(Utils::getIntByte<uint64_t>(value, 4));
	serRes.push_back(Utils::getIntByte<uint64_t>(value, 3));
	serRes.push_back(Utils::getIntByte<uint64_t>(value, 2));
	serRes.push_back(Utils::getIntByte<uint64_t>(value, 1));
	serRes.push_back(Utils::getIntByte<uint64_t>(value, 0));
}

void Utils::addSerialized(uint32_t value, vector<uint8_t>& serRes) {

	serRes.push_back(Utils::getIntByte<uint32_t>(value, 3));
	serRes.push_back(Utils::getIntByte<uint32_t>(value, 2));
	serRes.push_back(Utils::getIntByte<uint32_t>(value, 1));
	serRes.push_back(Utils::getIntByte<uint32_t>(value, 0));
}

void Utils::addSerialized(uint16_t value, vector<uint8_t>& serRes) {

	serRes.push_back(Utils::getIntByte<uint16_t>(value, 1));
	serRes.push_back(Utils::getIntByte<uint16_t>(value, 0));
}

void Utils::addSerialized(float value, vector<uint8_t>& serRes) {

	int byteIdx;

	union u1 {
		float value;
		uint8_t byteMap[sizeof(float)];
	} floatMap;

	floatMap.value = value;

	for (byteIdx = sizeof(float)-1; byteIdx >= 0; byteIdx--) {
		serRes.push_back(floatMap.byteMap[byteIdx]);
	}
}

uint16_t Utils::float2ushort(float value) {

	value = roundf(value);
	if (value < 0.0f) {
		return 0;
	}
	else {
		if (value > 65535.0f) {
			return 65535;
		}
		else {
			return (uint16_t (value));
		}
	}
}

std::string Utils::lwHexEncode(unsigned char v) {
	static const char hexChars[] = "0123456789ABCDEF";
	std::string o = "0x";
	o += hexChars[v/16];
	o += hexChars[v%16];
	return o;
}

////////////////////////////////////////////////
// BENCHMARK
std::vector<std::tuple<std::string, long, int> > Benchmark::values = std::vector<std::tuple<std::string, long, int> >();
unsigned Benchmark::level = 0;

long Benchmark::getMicros() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
}

Benchmark::Benchmark(const std::string& name) : t0(0), name(name) {
	t0 = 0;
}

void Benchmark::start() {
	t0 = getMicros();
	values.push_back(std::make_tuple(name, 0, level++));
}

void Benchmark::stop() {
	long t1 = getMicros();
	values.push_back(std::make_tuple(name, t1-t0, --level));
}

std::string Benchmark::getResults() {
	const unsigned timeWidth = 10;
	std::stringstream ss;
	std::vector<std::tuple<std::string, long, int> >::iterator it;
	long total = 0;
	for (it = values.begin(); it != values.end(); ++it) {
		const std::string& name = std::get<0>(*it);
		long dt = std::get<1>(*it);
		int lvl = std::get<2>(*it);
		if (dt) {
			ss << std::setw(timeWidth) << dt << "us " << std::string(lvl,'-') << "> " << name << std::endl;
		} else {
			ss << std::string(timeWidth-3, ' ') << "START " << std::string(lvl,'-') << "> " << name << std::endl;
		}
		if (lvl == 0)
			total += dt;
	}
	ss << "Total time measured: " << total << "us\n";
	return ss.str();
}

void Benchmark::writeResults(const std::string& filename) {
	std::ofstream fo(filename.c_str());
	if (!fo.good()) { std::cout << "Error while writing benchmark to file\n"; return; }
	fo << getResults();
	fo.close();
}

} /* namespace sniutils */
