#include <cstdio>
#include <string>
#include <sstream>
#include <fstream>

#include <iostream>
#include <stdexcept>

#include "ConfigManager.h"
#include "MeasProcessor.h"
#include "Utils.h"
#include "mainStuff.h"


using namespace sniutils;
using namespace sniprotocol;
using namespace sni;

const std::string benchmarkOutFile = "BENCHMARK.txt";
const std::string matlabOutFile = "plotRCBs.m";
const std::string jsonOutFile = "jsonString.json";

static const int initialTimestamp = 0; // The arrival time of the train

// RSB data
std::vector<tRSB> RSBs;

// RCB data
std::vector<tRCB> RCBs;

unsigned int numberOfSamples;

std::string jsonString;

int main(int argc, char *argv[]) {
	try {

		if (argc != 3) {
			cout << "Program requires two arguments! Expected call: erdm_offline <path to settings file> <path to rawdata file>" << endl;
			return -2;
	 	}

		std::string configfilePath(argv[1]);
		std::string inputDataFile(argv[2]);

		{
			Benchmark b("Loading input from file");
			b.start();
			loadInputData(inputDataFile, numberOfSamples, RSBs, RCBs, initialTimestamp);
			b.stop();
		}

		{
			Benchmark b("DSP algorithm");
			b.start();
			preprocessDSP(initialTimestamp, RSBs, RCBs, jsonString);
			b.stop();
		}

		{
			Benchmark b("SNI algorithm");
			b.start();
			processSNI(configfilePath, jsonString);
			b.stop();
		}
		Benchmark::writeResults(benchmarkOutFile);

		//exportInputDataToML(matlabOutFile, numberOfSamples, RCBs); // Show loaded raw data in MATLAB
		saveJsonString(jsonOutFile, jsonString); // Save json string to file
	}
	catch(const std::exception& ex) {
		std::cout << "ERROR: " << ex.what() << std::endl;
		return -1;
	}

	return 0;
}
