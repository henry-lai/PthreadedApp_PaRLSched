#ifndef ERDM_PC_RSB_H_
#define ERDM_PC_RSB_H_

#include <vector>
#include "Utils.h"

typedef struct {
	int Pl, Ts;
} tPulse;

class RSBProc {
	unsigned char address;
	int temperature;
	int pulseStart; // Time when the axle arrived
	int pulseLength; // Time the axle spent above sensor
	int t0; // Initial timestamp
	int axleNo; // Number of axle
	std::string outputS;
public:
	std::vector<unsigned char> rawData;
	std::vector<tPulse> pulses;

	RSBProc();
	RSBProc(unsigned char addr, int initialTimestamp = 0, int temp = 200);
	void setSize(unsigned int size);
	void loadData(unsigned char data, unsigned int pos);
	void processData();
	const std::string& getJson();
};

#endif /* ERDM_PC_RSB_H_ */
