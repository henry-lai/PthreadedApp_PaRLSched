#include "Scheduler.h"
#include "ThreadInfo.h"
#include "ThreadControl.h"
#include "TestParams.h"
#include <vector>
#include <ctime>
#include <sys/time.h>
#include <sched.h>
#include <error.h>
#include <unistd.h>
#include <vector>
#include <errno.h>
#include <algorithm>
#include <numeric>
#include <cmath>


#include <cstdlib>
#include <cstring>

#include <iostream>
#include <fstream>

#include <math.h>
#include <assert.h>
#include <float.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>


/*Scheduler Parameters*/
//#define STEP_SIZE					0.01							// the standard one is 0.005
//#define LAMBDA						0.02							// the standard one is 0.2
//#define SCHED_PERIOD				0.50
//#define OS_MAPPING					0
//#define RL_MAPPING					1
#define PR_MAPPING					0								// This predefined mapping assigns half threads to the first NUMA node, and the rest to the second one.
#define ST_MAPPING					0
#define OPTIMIZE_MAIN_RESOURCE		1
#define SMART_MALLOC				0								// It is used for memory allocation in some applications.
#define SUSPEND_THREADS				0								// if 1, threads are suspended before re-allocated
//#define GAMMA						0.02							// a parameter related to minimizing VARIANCE
#define RL_ACTIVE_RESHUFFLING		0
#define RL_PERFORMANCE_RESHUFFLING	1

/*Output Parameters*/
#define PRINTOUT_STRATEGIES			0
#define PRINTOUT_ACTIONS			1

/*Testing Parameters (may only be used when THREAD_COUNT=4*/
#define WRITE_TO_FILES				0								// enables printing outcome to files
#define WRITE_TO_FILES_DETAILS		0								// enables printing outcome to files (including strategy updates)

double step_size 					= STEP_SIZE;					/* Step-size of Reinforcement Learning Algorithm */
double lambda 						= LAMBDA;
bool printout_strategies			= PRINTOUT_STRATEGIES;
bool printout_actions				= PRINTOUT_ACTIONS;
bool write_to_files 				= WRITE_TO_FILES;
bool write_to_files_details			= WRITE_TO_FILES_DETAILS;
double sched_period 				= SCHED_PERIOD;	// in nanoseconds
bool OS_mapping 					= OS_MAPPING;
bool RL_mapping						= RL_MAPPING;
bool PR_mapping						= PR_MAPPING;
bool ST_mapping						= ST_MAPPING;
bool optimize_main_resource			= OPTIMIZE_MAIN_RESOURCE;
bool SMART_malloc					= SMART_MALLOC;
double gamma_par					= GAMMA;
bool suspend_threads				= SUSPEND_THREADS;
bool RL_active_reshuffling			= RL_ACTIVE_RESHUFFLING;
bool RL_performance_reshuffling		= RL_PERFORMANCE_RESHUFFLING;

void initScheduler(Scheduler &scheduler, unsigned int numThreads)
{
//  std::vector< std::string > RESOURCES = {"NUMA_PROCESSING","NUMA_MEMORY"};
//  std::vector< std::string > RESOURCES_EST_METHODS = {"AL", "RL"};
//  std::vector< std::string > RESOURCES_OPT_METHODS = {"AL", "RL"};
//  std::vector< std::string > CHILD_RESOURCES = {"CPU_PROCESSING","NULL"};
//  std::vector< std::string > CHILD_RESOURCES_EST_METHODS = {"RL", "RL"};
//  std::vector< std::string > CHILD_RESOURCES_OPT_METHODS = {"RL", "RL"};
//
//  std::vector< unsigned int > MAX_NUMBER_MAIN_RESOURCES = { 2 , 1 };
//
//  std::vector< std::vector< unsigned int > > MAX_NUMBER_CHILD_RESOURCES = { { 14, 14 }, { 0 , 0 } };

	Scheduler scheduler_tmp(numThreads);

	
	scheduler = scheduler_tmp;

	fprintf(stderr, "Scheduler is %p\n", &scheduler);

  
//  scheduler.initialize(
//		       numThreads
//		       , RESOURCES
//		       , RESOURCES_EST_METHODS
//		       , RESOURCES_OPT_METHODS
//		       , MAX_NUMBER_MAIN_RESOURCES
//		       , CHILD_RESOURCES
//		       , CHILD_RESOURCES_EST_METHODS
//		       , CHILD_RESOURCES_OPT_METHODS
//		       , MAX_NUMBER_CHILD_RESOURCES
//		       , RL_active_reshuffling
//		       , RL_performance_reshuffling
//		       , step_size
//		       , lambda
//		       , sched_period
//		       , suspend_threads
//		       , printout_strategies
//		       , printout_actions
//		       , write_to_files
//		       , write_to_files_details
//		       , gamma_par
//		       , OS_mapping
//		       , RL_mapping
//		       , PR_mapping
//		       , ST_mapping
//			   , optimize_main_resource
//		       );
  
}

