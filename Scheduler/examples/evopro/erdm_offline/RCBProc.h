#ifndef ERDM_PC_RCB_H_
#define ERDM_PC_RCB_H_

#include <vector>

#include "FIR.h"
#include "Utils.h"

const unsigned int filterLength = 52;
const float massFactor = 16384; // Mass is stored in floats between 0 and 1. Multiplied by this value, you can get the real mass.
const float massOn = 2000; // Turn on trigger when mass is above this value (kilograms)
const float massOff = 1800; // Turn off trigger when mass is below this value (kilograms) ! This must be smaller than massOn



const float thr = massOn/massFactor;
const float hys = (massOn-massOff)/massFactor;
// Turn on at level thr
// Turn off at level thr-hys

static const float bandpass_coefs[filterLength] = {
#include "Bp_coefs.dat"
};
static const float hilbert_coefs[filterLength] = {
#include "Hil_coefs.dat"
};

typedef struct {
	std::vector<int> samples;
	int pl;
	int Ts;
} tDecimatedData;

class RCBProc {
	unsigned char address;
	int temperature;
	FIR bp, hil; // FIlters
	int t0; // Initial timestamp
	std::string outputS;
	void decimate(std::vector<int>& out, int start, int len);
public:
	std::vector<float> rawData; // raw samples
	std::vector<float> filteredData; // Samples after filtering
	std::vector<tDecimatedData> decimatedData; // Samples after decimation

	RCBProc();
	RCBProc(unsigned char addr, int initialTimestamp = 0, int temp = 200);
	void setSize(unsigned int size);
	void loadData(float data, int pos);
	void filterData();
	void triggerAndDecimateData();
	const std::string& getJson();
};

#endif /* ERDM_PC_RCB_H_ */
