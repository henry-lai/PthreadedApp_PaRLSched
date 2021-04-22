#ifndef MAINSTUFF_H_
#define MAINSTUFF_H_

#include <string>
#include <vector>

#include "RSBProc.h"
#include "RCBProc.h"

// RSB data
typedef struct {
	unsigned char addr;
	int sensPos0;
	int sensPos1;
} tRSBProperties; // This struct must be identical in the function, which writes the .dat file
typedef struct {
	tRSBProperties props;
	RSBProc rsbProc;
} tRSB;

// RCB data
typedef struct {
	unsigned char addr;
	int position;
	bool left;
} tRCBProperties; // This struct must be identical in the function, which writes the .dat file
typedef struct {
	tRCBProperties props;
	RCBProc rcbProc;
} tRCB;


void loadInputData(const std::string& filename, unsigned int& numberOfSamples, std::vector<tRSB>& RSBs, std::vector<tRCB>& RCBs,const int initialTimestamp);
void exportInputDataToML(const std::string& filename, unsigned int numberOfSamples, const std::vector<tRCB>& RCBs);
void preprocessDSP(const int initialTimestamp, std::vector<tRSB>& RSBs, std::vector<tRCB>& RCBs, std::string& jsonString);
void saveJsonString(const std::string& filename, const std::string& jsonString);
void processSNI(const std::string& configFilePath,const std::string& jsonString);


#endif /* MAINSTUFF_H_ */
