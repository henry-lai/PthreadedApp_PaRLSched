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
#include "TaskQueue.hxx"

#ifdef SCHEDULER
#include <pthread.h>
#include "SchedulerParams.h"
Scheduler scheduler;
pthread_t sched_thread;
#endif

using namespace sniutils;
using namespace sniprotocol;
using namespace sni;

const std::string benchmarkOutFile = "BENCHMARK.txt";
const std::string matlabOutFile = "plotRCBs.m";
const std::string jsonOutFile = "jsonString.json";

int nthreads;
Context context;

static const int initialTimestamp = 0; // The arrival time of the train

// RSB data
std::vector<tRSB> RSBs;

// RCB data
std::vector<tRCB> RCBs;

unsigned int numberOfSamples;

std::string jsonString;

#ifdef SCHEDULER
void initScheduler(Scheduler &scheduler) 
{
  std::vector< std::string > RESOURCES = {"NUMA_PROCESSING","NUMA_MEMORY"};
  std::vector< std::string > RESOURCES_EST_METHODS = {"AL", "RL"};
  std::vector< std::string > RESOURCES_OPT_METHODS = {"AL", "RL"};
  std::vector< std::string > CHILD_RESOURCES = {"CPU_PROCESSING","NULL"};
  std::vector< std::string > CHILD_RESOURCES_EST_METHODS = {"RL", "RL"};
  std::vector< std::string > CHILD_RESOURCES_OPT_METHODS = {"RL", "RL"};

  std::vector< unsigned int > MAX_NUMBER_MAIN_RESOURCES = { 2 , 1 };

  std::vector< std::vector< unsigned int > > MAX_NUMBER_CHILD_RESOURCES = { { 8, 8 }, { 0 , 0 } };

  scheduler.initialize(
		       nthreads
		       , RESOURCES
		       , RESOURCES_EST_METHODS
		       , RESOURCES_OPT_METHODS
		       , MAX_NUMBER_MAIN_RESOURCES
		       , CHILD_RESOURCES
		       , CHILD_RESOURCES_EST_METHODS
		       , CHILD_RESOURCES_OPT_METHODS
		       , MAX_NUMBER_CHILD_RESOURCES
		       , RL_active_reshuffling
		       , RL_performance_reshuffling
		       , step_size
		       , lambda
		       , sched_period
		       , suspend_threads
		       , printout_strategies
		       , printout_actions
		       , write_to_files
		       , write_to_files_details
		       , gamma_par
		       , OS_mapping
		       , RL_mapping
		       , PR_mapping
		       , ST_mapping
		       );
  
}
#endif

int main(int argc, char *argv[]) {
  try {
    
    if (argc != 4) {
      cout << "Program requires three arguments! Expected call: erdm_pthreads_sched <nr_threads> <path to settings file> <path to rawdata file>" << endl;
      return -2;
    }
    
    std::string configfilePath(argv[2]);
    std::string inputDataFile(argv[3]);
    nthreads = atoi(argv[1]);

#ifdef SCHEDULER
  initScheduler(scheduler);
#endif 


    context.setNumThreads(nthreads);
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
