#include <fstream>
#include <stdexcept>
#include <iostream>
#include <sstream>

#include "mainStuff.h"
#include "Utils.h"
#include "RSBProc.h"
#include "RCBProc.h"
#include "ConfigManager.h"
#include "MeasProcessor.h"

#include "TaskQueue.hxx"

extern Context context;

//#define DEBUG 0

//#define REFINE_CURVES 1

void loadInputData(const std::string& filename, unsigned int& numberOfSamples, std::vector<tRSB>& RSBs, std::vector<tRCB>& RCBs, const int initialTimestamp) {
	std::ifstream fi(filename.c_str(), std::ios::binary | std::ios::in);
	char errorStr[1024];
	unsigned int i, j;
	unsigned numberOfRSBs, numberOfRCBs;
	unsigned char c;
	float f;
	if (!fi.good()) {
		sprintf(errorStr, "File '%s' cannot be opened", filename.c_str());
		throw std::runtime_error(errorStr);
	}
	fi.exceptions( std::ifstream::failbit | std::ifstream::badbit );
	try {
		fi.read((char*)&numberOfSamples, sizeof(numberOfSamples)); // Read number of samples
		// Read RSBs
		fi.read((char*)&numberOfRSBs, sizeof(numberOfRSBs));
		std::cout << "There are " << numberOfRSBs << " RSBs in the data file\n";
		RSBs.clear();
		for (i = 0; i < numberOfRSBs; ++i) {
			RSBs.push_back(tRSB());
			tRSB& RSB = RSBs.back();
			tRSBProperties& RSBprops = RSB.props;
			fi.read((char*)&RSBprops, sizeof(RSBprops));
//#if DEBUG
//			std::cout << "*RSB: addr=" << (int)(RSBprops.addr) << ", senspos0=" << RSBprops.sensPos0 << "mm, senspos1=" << RSBprops.sensPos1 << "mm\n";
//#endif
			RSB.rsbProc = RSBProc(RSBprops.addr, initialTimestamp);
			RSB.rsbProc.setSize(numberOfSamples);
			for (j = 0; j < numberOfSamples; ++j) {
				fi.read((char*)&c, sizeof(c));
				RSB.rsbProc.loadData(c, j);
			}
		}
		// Read RCBs
		fi.read((char*)&numberOfRCBs, sizeof(numberOfRCBs));
		std::cout << "There are " << numberOfRCBs << " RCBs in the data file\n";
		RCBs.clear();
		for (i = 0; i < numberOfRCBs; ++i) {
			RCBs.push_back(tRCB());
			tRCB& RCB = RCBs.back();
			tRCBProperties& RCBprops = RCB.props;
			fi.read((char*)&RCBprops, sizeof(RCBprops));
//#if DEBUG
//			std::cout << "*RCB: addr=" << (int)(RCBprops.addr) << ", position=" << RCBprops.position << "mm, it is on the ";
//			if (RCBprops.left) std::cout << "left rail\n";
//			else std::cout << "right rail\n";
//#endif
			RCB.rcbProc = RCBProc(RCBprops.addr, initialTimestamp);
			RCB.rcbProc.setSize(numberOfSamples);
			for (j = 0; j < numberOfSamples; ++j) {
				fi.read((char*)&f, sizeof(f));
				RCB.rcbProc.loadData(f, j);
			}
		}
		int filepos = fi.tellg();
		fi.seekg(0, fi.end);
		int filesize = fi.tellg();
		if (filepos != filesize) {
			std::cout << "WARNING: We have not reached the end of the file. The input file might be corrupted.\n";
		}
		fi.close();
	} catch (const std::ios::failure& ex) {
		long fpos = fi.tellg();
		sprintf(errorStr, "File corrupted, approximate position: %ld, what: %s", fpos, ex.what());
		throw std::runtime_error(errorStr);
	}
}

void exportInputDataToML(const std::string& filename, unsigned int numberOfSamples, const std::vector<tRCB>& RCBs) {
	std::stringstream legL;
	std::stringstream legR;
	unsigned int i, j;
	unsigned numberOfRCBs = RCBs.size();
	legL << "legend(";
	legR << "legend(";
	std::ofstream fo(filename.c_str());
	fo << "x=1:" << (numberOfSamples+1) << ";\nz=zeros(1," << (numberOfSamples+1) << ");\nfigure(1);hold all;title('RCBs on the right');\nfigure(2);hold all;title('RCBs on the left');\n";
	for (i = 0; i < numberOfRCBs; ++i) {
		const tRCBProperties& RCB = RCBs[i].props;
		fo << "% RCB 0x" << std::hex << (int)RCB.addr << "\nz=z+1;\nfigure(" << ((RCB.left)?2:1) << ");\n";
		std::stringstream& leg = (RCB.left)?legL:legR;
		leg << "'0x" << std::hex << (int)RCB.addr << "',";
		fo << "y=[";
		const std::vector<float>& data = RCBs[i].rcbProc.rawData;
		for (j = 0; j < numberOfSamples; ++j) {
			fo << data[j] << ",";
		}
		fo << "0];\nplot3(x,y,z);\n";
	}
	std::string s = legL.str();
	s.erase(s.length()-1);
	fo << "figure(2);" << s << ");\n";
	s = legR.str();
	s.erase(s.length()-1);
	fo << "figure(1);" << s << ");\n";
	fo.close();
}

void preprocessDSP(const int initialTimestamp, std::vector<tRSB>& RSBs, std::vector<tRCB>& RCBs, std::string& jsonString) {
	unsigned numberOfRSBs = RSBs.size();
	unsigned numberOfRCBs = RCBs.size();

	// RSB
	{
		Benchmark b("RSB processing");
		b.start();
		for (unsigned i = 0; i < numberOfRSBs; ++i) {
			tRSB& current = RSBs[i];
			RSBProc& rsb = current.rsbProc;
			rsb.processData();
		}
		b.stop();
	}

	// RCB
	{
		Benchmark b("RCB filtering");
		std::cout << "RCB filtering\n";
		b.start();

		for (unsigned i = 0; i < numberOfRCBs; ++i) {
			tRCB& current = RCBs[i];
			RCBProc& rcb = current.rcbProc;
			context.setRCBProc(rcb);
			double startTime1= Utils::getTimeSec();
			rcb.filterData();
			double endTime= Utils::getTimeSec();
			std::cout <<"Kernel1 run time:" << endTime - startTime1 << "sec" << std::endl << std::endl;
		}

		b.stop();
	}

	{
		Benchmark b("RCB decimation");
		std::cout << "RCB decimating\n";
		b.start();
		for (unsigned i = 0; i < numberOfRCBs; ++i) {
			tRCB& current = RCBs[i];
			RCBProc& rcb = current.rcbProc;
			rcb.triggerAndDecimateData();
		}

		b.stop();
	}

	// Replace decimated curves with a real curve
	{
//#if REFINE_CURVES
//		Benchmark b("Real curve");
//		b.start();
//		static const std::vector<int> mul = {
//			436,468,459,475,499,539,584,647,720,802,884,947,988,1019,1039,1054,
//			1069,1086,1099,1106,1105,1103,1094,1082,1072,1074,1081,1078,1074,1072,1069,1068,
//			1065,1051,1030,1001,973,955,949,957,964,974,980,977,970,958,945,928,
//			917,897,869,832,787,744,702,659,625,608,594,601,639,682,766,847
//		};
//		for (tRCB& rcb: RCBs) {
//			RCBProc& rcbProc = rcb.rcbProc;
//			for (tDecimatedData& decimatedData: rcbProc.decimatedData) {
//				std::vector<int>& samples = decimatedData.samples;
//				for (int i = 0; i < 64; ++i) {
//					samples[i] = samples[i] * mul[i] / 1024;
//				}
//			}
//		}
//		b.stop();
//#endif
	}

	// JSON
	{
		Benchmark b("Convert data to JSON");
		b.start();
		jsonString = "{\n \"RCBs\":[\n";
		for (unsigned i = 0; i < numberOfRCBs-1; ++i)
			jsonString += RCBs[i].rcbProc.getJson() + ",\n";
		jsonString += RCBs.back().rcbProc.getJson() + "\n ],\n \"RSBs\":[\n";
		for (unsigned i = 0; i < numberOfRSBs-1; ++i)
			jsonString += RSBs[i].rsbProc.getJson() + ",\n";
		jsonString += RSBs.back().rsbProc.getJson() + "\n ]\n}";
		b.stop();
	}
}

void saveJsonString(const std::string& filename, const std::string& jsonString){
	std::ofstream fo(filename.c_str());
	fo << jsonString;
	fo.close();
}

void processSNI(const std::string& configFilePath,const std::string& jsonString) {
	sni::ConfigManager confMgr(Logger::getL(), configFilePath);
	confMgr.addMeasString(jsonString);
	sni::MeasProcessor measProc(confMgr, Logger::getL());
	measProc.doProcessing();
	std::cout << measProc;
}


