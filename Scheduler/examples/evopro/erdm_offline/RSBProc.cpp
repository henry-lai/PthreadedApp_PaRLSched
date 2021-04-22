#include <sstream>
#include <cmath>
#include <iostream>

#include "RSBProc.h"

RSBProc::RSBProc()
	: address(0),
	  temperature(0),
	  pulseStart(0),
	  pulseLength(-1),
	  t0(0),
	  axleNo(0) {}

RSBProc::RSBProc(unsigned char addr, int initialTimestamp, int temp)
	: address(addr),
	  temperature(temp),
	  pulseStart(0),
	  pulseLength(-1),
	  t0(initialTimestamp),
	  axleNo(0) {}

void RSBProc::setSize(unsigned int size) {
	rawData = std::vector<unsigned char>(size, 0);
}

void RSBProc::loadData(unsigned char data, unsigned int pos) {
	rawData[pos] = data;
}

void RSBProc::processData() {
	int pulseLength = -1;
	int pulseStart;
	for (unsigned int i = 0; i < rawData.size(); ++i) {
		if (rawData[i]) {
			if (pulseLength == -1) { // Pulse starts now
				pulseStart = t0 + i;
			}
			pulseLength++;
		} else {
			if (pulseLength != -1) { // Pulse stops now
				pulses.push_back(tPulse());
				pulses.back().Pl = round(pulseLength/6.25);
				pulses.back().Ts = round((pulseStart)/6.25);
			}
			pulseLength = -1;
		}
	}
}

const std::string& RSBProc::getJson() {
	std::stringstream ss;
	int l = pulses.size();
	int i;
	ss << "  {\"Add\":\"" << sniutils::Utils::lwHexEncode(address) << "\",\"ID\":" << (int)address << ",\"NumE\":" << l << ",\"E\":\n   [\n";
	for (i = 0; i < l; ++i) {
		ss << "    {\"Ax\":" << axleNo++ << ",\"Pl\":[" << pulses[i].Pl << ",0],\"Stat\":[0,1],\"T\":[" << temperature << ",0],\"Ts\":[" << pulses[i].Ts << ",0]}";
		if (i != l-1) ss << ",";
		ss << "\n";
	}
	ss << "   ]\n  }";
	outputS = ss.str();
	return outputS;
}
