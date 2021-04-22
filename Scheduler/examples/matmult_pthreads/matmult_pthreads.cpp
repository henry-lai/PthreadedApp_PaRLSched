/* -----------------------------------------------------------------------------
 * Copyright (C) 2012-2014 Software Competence Center Hagenberg GmbH (SCCH)
 * <georgios.chasparis@scch.at>, <office@scch.at>
 * -----------------------------------------------------------------------------
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -----------------------------------------------------------------------------
 * This code is subject to dual-licensing. Please contact office@scch.at
 * if you are interested in obtaining a differently licensed version.
*/

//============================================================================
// Name        : SchedulingExample.cpp
// Author      : Georgios Chasparis
// Version     : 1.2
// Date		   : 05.12.2016
// Description : This example creates a simple parallel application consisting of
//				 of a number of parallel threads executing identical routines.
//				 In parallel, the PaRL-sched is being called, which is responsible
//				 for allocating threads to the available CPU cores. The allocation of
//				 threads into the available CPU cores is based upon a Reinforcement-Learning
//				 strategy update rule.
//============================================================================

#include <iostream>
#include <vector>
#include <ctime>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#include <error.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <unistd.h>
#include <vector>
#include <assert.h>
#include <errno.h>
#include <algorithm>
#include <numeric>
#include <cmath>

#include <math.h>
#include <sched.h>
#include <numa.h>
// #include <hwloc.h>

#define handle_error_en(en, msg) \
		do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)


#include "ThreadInfo.h"
#include "ThreadControl.h"
#include "Scheduler.h"

//#include "ThreadRoutine.h"

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>

/*
 * Input Parameters
 */

/*Scheduler Parameters*/
#define STEP_SIZE					0.01							// the standard one is 0.005
#define LAMBDA						0.02							// the standard one is 0.2
#define SCHED_PERIOD				0.20
#define OS_MAPPING					0
#define RL_MAPPING					1
#define PR_MAPPING					0								// This predefined mapping assigns half threads to the first NUMA node, and the rest to the second one.
#define ST_MAPPING					0
#define OPTIMIZE_MAIN_RESOURCE		0
#define SMART_MALLOC				0								// It is used for memory allocation in some applications.
#define SUSPEND_THREADS				0								// if 1, threads are suspended before re-allocated
#define GAMMA						0.02							// a parameter related to minimizing VARIANCE
#define RL_ACTIVE_RESHUFFLING		0
#define RL_PERFORMANCE_RESHUFFLING	1

/*Output Parameters*/
#define PRINTOUT_STRATEGIES			0
#define PRINTOUT_ACTIONS			1

/*Testing Parameters (may only be used when THREAD_COUNT=4*/
#define WRITE_TO_FILES				0								// enables printing outcome to files
#define WRITE_TO_FILES_DETAILS		0								// enables printing outcome to files (including strategy updates)

/*Declaration of Input Parameters*/
int dim;
int thread_count;
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




double **a;
double **b;
double **res;


double get_current_time()
{
  static int start = 0, startu = 0;
  struct timeval tval;
  double result;


  if (gettimeofday(&tval, NULL) == -1)
    result = -1.0;
  else if(!start) {
    start = tval.tv_sec;
    startu = tval.tv_usec;
    result = 0.0;
  }
  else
    result = (double) (tval.tv_sec - start) + 1.0e-6*(tval.tv_usec - startu);

  return result;
}

// #define OVER_FMT "handler(%d ) Overflow at %p! bit=0x%llx \n"
// #define ERROR_RETURN(retval) { fprintf(stderr, "Error %d %s:line %d: \n", retval,__FILE__,__LINE__); exit(retval); }

#define handle_error_en(en, msg) \
		do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

double multiply_row_by_column (double **mat1, int row, double **mat2, int col)
{
  int k;
  double sum=0;
  for (k=0; k<dim; k++)
    sum += mat1[row][k] * mat2[k][col];
}


void multiply_row_by_matrix (double **mat1, int row, double **mat2, double **res)
{
  for (int col=0; col<dim; col++)
    res[row][col] = multiply_row_by_column (mat1, row, mat2, col);

}

static void * thread_execute (void* arg)
{

  /*
   * Initializing performance counters
   */
  ThreadControl thread_control;
  thread_info * info = (static_cast<thread_info*>(arg));
  if(!thread_control.thd_init_counters (info->thread_id, arg))
    printf("Error: Problem initializing counters for thread %d",info->thread_num);

  /*
   * Get the pid of the process
   */
  info->tid = syscall(SYS_gettid);
  // std::cout << " 	Thread " << info->tid << " runs on CPU " << sched_getcpu() << std::endl;



  // Ant-Colony Optimization
  for (unsigned int i = info->thread_ref; i < info->thread_ref + info->thread_goal; i++){
	  multiply_row_by_matrix(a, i, b, res);
  }

  info->status = 1;
  info->termination_time = info->time_before - info->time_init;
  return (void *)0;
}


void thread_create(thread_info* tinfo)
{
  pthread_attr_t detach;
  int status;
  int i;

  /*
   * The detach state attribute determines whether a thread created
   * using the thread attributes object 'attr' will be created in a joinable or a
   * detach state.
   *
   */
  status = pthread_attr_init(&detach);
  if (status != 0)
    // err_abort (status, "Init attributes object");
    std::cout << "Init attributes object" << std::endl;

  status = pthread_attr_setdetachstate(&detach, PTHREAD_CREATE_DETACHED);
  if (status != 0)
    std::cerr << "Set create-detached" << std::endl;

  int chunk_size = dim / thread_count;
  int extra_pool = dim - (thread_count * chunk_size);
  int count = 0;

  for (unsigned int thread_ind = 0; thread_ind < thread_count; thread_ind++) {
    /* Setting some generic parameters for each thread */
    tinfo[thread_ind].thread_num = thread_ind;
    int extra;
    if (extra_pool>0) {
      extra = 1;
      extra_pool--;
    } else {
      extra = 0;
    }
    int chunk = chunk_size + extra;
    tinfo[thread_ind].thread_ref = count;
    tinfo[thread_ind].thread_goal = chunk;
    count += chunk;

    std::cout << " thread " << thread_ind << " has reference " << tinfo[thread_ind].thread_ref << " and goal " << tinfo[thread_ind].thread_goal << std::endl;

    tinfo[thread_ind].status = 0;

    /*
     * For each thread to be created, we assign a version of ThreadRoutine.
     * This can be different for each thread, however here is an idendical function.
     */
    // ThreadRoutine thread_routine();
    status = pthread_create(&tinfo[thread_ind].thread_id, &detach, &thread_execute, &tinfo[thread_ind]);
    if (status != 0)
      handle_error("Creating thread failed!");

    std::cout << " thread_id of thread " << thread_ind << " is " << tinfo[thread_ind].return_thread_id() << std::endl;
  }
}


int main (int argc, char *argv[])
{
  if (argc<3) {
    std::cerr << "Usage: SchedulingExample <num_workers> <matrix_dim>" <<"\n";
    exit(1);
  }
  dim = atoi(argv[2]);
  int num_workers = atoi(argv[1]);

  a = (double **) malloc (sizeof(double *) * dim);
  b = (double **) malloc (sizeof(double *) * dim);
  res = (double **) malloc (sizeof(double *) * dim);
  for (int i=0; i<dim; i++) {
    a[i] = (double *) malloc (sizeof(double) * dim);
    b[i] = (double *) malloc (sizeof(double) * dim);
    res[i] = (double *) malloc (sizeof(double) * dim);
    for (int j=0; j<dim; j++) {
      a[i][j] = 42.0;
      b[i][j] = 42.0;
    }
  }

  double t1 = get_current_time();

  thread_count = num_workers;

//  /*
//   * Let us assume that we receive a vector of resources that need to be allocated (optimized) at any
//   * given time. For example, one such resource is PROCESSING_BANDWIDTH, or MEMORY, etc.
//   * This information should be automatically retrieved from the config file of the scheduler.
//   */
//  std::vector< std::string > RESOURCES = {"NUMA_PROCESSING","NUMA_MEMORY"};
//  std::vector< std::string > RESOURCES_EST_METHODS = {"AL", "RL"};
//  std::vector< std::string > RESOURCES_OPT_METHODS = {"AL", "RL"};
//  std::vector< std::string > CHILD_RESOURCES = {"CPU_PROCESSING","NULL"};
//  std::vector< std::string > CHILD_RESOURCES_EST_METHODS = {"RL", "RL"};
//  std::vector< std::string > CHILD_RESOURCES_OPT_METHODS = {"RL", "RL"};
//
//  /*
//   * At this point, we need to also define the corresponding criteria (i.e., utility functions)
//   * based on which the optimization is performed. Such a criterion should be performed for each one
//   * of the resources to be optimized. When optimizing over Processing Bandwidth, such a criterion may
//   * correspond to the processing speed, however alternative criteria may be defined.
//   */
//
//  /*
//   * Maximum allowable number of resources
//   */
//  std::vector< unsigned int > MAX_NUMBER_MAIN_RESOURCES = { 2 , 1 };
//
//  std::vector< std::vector< unsigned int > > MAX_NUMBER_CHILD_RESOURCES = { { 8, 8 }, { 0 , 0 } };

  /*
   * Initialize Scheduler
   *
   * @inputs:
   * 		- Types of Resources to be scheduled (e.g., CPU bandwidth, Memory, etc.)
   * 		- In NUMA architectures, we should be able to select node for a running thread.
   */
  Scheduler scheduler(thread_count);


  /*
   * Defining Threads / Parallelization Pattern
   *
   * @description: This part might be different for each application and it is up to the user to define properly these threads.
   * The main important input for the PaRLSched library is to get access to the id's of the threads defined.
   */
  thread_create( scheduler.get_tinfo() );

    /*
     * Run Scheduler
     *
     * @description: This function call initiates the scheduler, which involves the pre-defined resource allocation
     *
     */
  scheduler.run();

  double t2 = get_current_time();
  std::cout << "Total execution time: " << t2-t1 << "\n";

  pthread_exit ( NULL );        /* Let threads finish */


}

// The following commented part of the code, is the same as above, just uses different pointers for each NUMA node

/*

double **a, **a1, **a2;
double **b, **b1, **b2;
double **res, **res1, **res2;


double get_current_time()
{
  static int start = 0, startu = 0;
  struct timeval tval;
  double result;
  

  if (gettimeofday(&tval, NULL) == -1)
    result = -1.0;
  else if(!start) {
    start = tval.tv_sec;
    startu = tval.tv_usec;
    result = 0.0;
  }
  else
    result = (double) (tval.tv_sec - start) + 1.0e-6*(tval.tv_usec - startu);
  
  return result;
}

// #define OVER_FMT "handler(%d ) Overflow at %p! bit=0x%llx \n"
// #define ERROR_RETURN(retval) { fprintf(stderr, "Error %d %s:line %d: \n", retval,__FILE__,__LINE__); exit(retval); }

#define handle_error_en(en, msg) \
		do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

double multiply_row_by_column (double **mat1, int row, double **mat2, int col)
{
  int k;
  double sum=0;
  for (k=0; k<dim; k++)
    sum += mat1[row][k] * mat2[k][col];
}


void multiply_row_by_matrix (double **mat1, int row, double **mat2, double **res)
{
  for (int col=0; col<dim; col++)
    res[row][col] = multiply_row_by_column (mat1, row, mat2, col);
  
}

static void * thread_execute (void* arg)
{


	   * Initializing performance counters

	  ThreadControl thread_control;
	  thread_info * info = (static_cast<thread_info*>(arg));
	  if(!thread_control.thd_init_counters(info->thread_id, arg))
		printf("Error: Problem initializing counters for thread %d",info->thread_num);

	  double t1 = get_current_time();


	   * Get the pid of the process

	  info->pid = syscall(SYS_getpid);
	  info->tid = syscall(SYS_gettid);
	  std::cout << "    Thread # " << info->thread_num << std::endl;
	  std::cout << " 	Thread " << info->tid << " with pid " << info->pid << " and ppid" << syscall(SYS_getppid) << "  and pthread_self() " << pthread_self() << std::endl;

	  // Matrix Multiplication
	  for (unsigned int i = info->thread_ref; i < info->thread_ref + info->thread_goal; i++)
	  {
		  // apply the following if PR = 1
		  if (PR_mapping){
			  if (SMART_malloc)
			  {
				  if (info->thread_num < 10)
					  multiply_row_by_matrix(a1, i, b1, res1);
				  else
					  multiply_row_by_matrix(a2, i, b2, res2);
			  }
			  else
			  {
				  // in this case only matrix at node 1 is being used for matrix multiplication
				  multiply_row_by_matrix(a1, i, b1, res1);
			  }
		  }
		  else if (RL_mapping)
		  {
			  if (info->memory_index == 0)
				  multiply_row_by_matrix(a, i, b, res);
			  else
				  multiply_row_by_matrix(a, i, b, res);
		  }
		  else if (OS_mapping)
			  multiply_row_by_matrix(a, i, b, res);
	  }

	  info->status = 1;
	  info->termination_time = info->time_before - info->time_init;

	  double t2 = get_current_time();

	  std::cout << " termination time of thread " << info->thread_num << " is " << t2-t1 << std::endl;
	  return (void *)0;
}


void thread_create(thread_info* tinfo)
{

	pthread_attr_t detach;
	int status;
	int i;
  

	   * The detach state attribute determines whether a thread created
	   * using the thread attributes object 'attr' will be created in a joinable or a
	   * detach state.
	   *

	  status = pthread_attr_init(&detach);
	  if (status != 0)
		// err_abort (status, "Init attributes object");
		std::cout << "Init attributes object" << std::endl;

	  status = pthread_attr_setdetachstate(&detach, PTHREAD_CREATE_DETACHED);
	  if (status != 0)
		std::cerr << "Set create-detached" << std::endl;

	  int chunk_size = dim / thread_count;
	  int extra_pool = dim - (thread_count * chunk_size);
	  int count = 0;

	  for (unsigned int thread_ind = 0; thread_ind < thread_count; thread_ind++) {
		 Setting some generic parameters for each thread
		tinfo[thread_ind].thread_num = thread_ind;
		int extra;
		if (extra_pool>0) {
		  extra = 1;
		  extra_pool--;
		} else {
		  extra = 0;
		}
		int chunk = chunk_size + extra;
		tinfo[thread_ind].thread_ref = count;
		tinfo[thread_ind].thread_goal = chunk;
		count += chunk;

		std::cout << " thread " << thread_ind << " has reference " << tinfo[thread_ind].thread_ref << " and goal " << tinfo[thread_ind].thread_goal << std::endl;

		// sleep(10);

		tinfo[thread_ind].status = 0;


		 * For each thread to be created, we assign a version of ThreadRoutine.
		 * This can be different for each thread, however here is an idendical function.

		// ThreadRoutine thread_routine();
		status = pthread_create(&tinfo[thread_ind].thread_id, &detach, &thread_execute, &tinfo[thread_ind]);
		if (status != 0)
		  handle_error("Creating thread failed!");

		std::cout << " thread_id of thread " << thread_ind << " is " << tinfo[thread_ind].return_thread_id() << std::endl;
	  }
}


int main (int argc, char *argv[])
{

	if (argc<3) {
	std::cerr << "Usage: SchedulingExample <num_workers> <matrix_dim>" <<"\n";
	exit(1);
	}
	dim = atoi(argv[2]);
	int num_workers = atoi(argv[1]);



	 * Allocating memory for the Matrices to be multiplied.

	a = (double **) malloc (sizeof(double *) * dim);
	b = (double **) malloc (sizeof(double *) * dim);
	res = (double **) malloc (sizeof(double *) * dim);
	for (int i=0; i<dim; i++) {
		a[i] = (double *) malloc (sizeof(double) * dim);
		b[i] = (double *) malloc (sizeof(double) * dim);
		res[i] = (double *) malloc (sizeof(double) * dim);
		for (int j=0; j<dim; j++) {
		  a[i][j] = 42.0;
		  b[i][j] = 42.0;
		}
	}


	// the following allocates matrices in 2 different NUMA nodes
	a1 = (double **) numa_alloc_onnode (sizeof(double *) * dim, 0);
	a2 = (double **) numa_alloc_onnode (sizeof(double *) * dim, 1);
	b1 = (double **) numa_alloc_onnode (sizeof(double *) * dim, 0);
	b2 = (double **) numa_alloc_onnode (sizeof(double *) * dim, 1);
	res1 = (double **) numa_alloc_onnode (sizeof(double *) * dim, 0);
	res2 = (double **) numa_alloc_onnode (sizeof(double *) * dim, 1);
	for (int i=0; i<dim; i++) {
	  a1[i] = (double *) numa_alloc_onnode (sizeof(double *) * dim, 0);
	  a2[i] = (double *) numa_alloc_onnode (sizeof(double *) * dim, 1);
	  b1[i] = (double *) numa_alloc_onnode (sizeof(double *) * dim, 0);
	  b2[i] = (double *) numa_alloc_onnode (sizeof(double *) * dim, 1);
	  res1[i] = (double *) numa_alloc_onnode (sizeof(double *) * dim, 0);
	  res2[i] = (double *) numa_alloc_onnode (sizeof(double *) * dim, 1);
	  for (int j=0; j<dim; j++) {
		a1[i][j] = 42.0;
		a2[i][j] = 42.0;
		b1[i][j] = 42.0;
		b2[i][j] = 42.0;
	  }
	}


	double t1 = get_current_time();

	thread_count = num_workers;


	* Let us assume that we receive a vector of resources that need to be allocated (optimized) at any
	* given time. For example, one such resource is PROCESSING_BANDWIDTH, or MEMORY, etc.
	* This information should be automatically retrieved from the config file of the scheduler.

	std::vector< std::string > RESOURCES = {"NUMA_PROCESSING","NUMA_MEMORY"};
	std::vector< std::string > RESOURCES_EST_METHODS = {"AL", "RL"};
	std::vector< std::string > RESOURCES_OPT_METHODS = {"AL", "RL"};
	std::vector< std::string > CHILD_RESOURCES = {"CPU_PROCESSING","NULL"};
	std::vector< std::string > CHILD_RESOURCES_EST_METHODS = {"RL", "RL"};
	std::vector< std::string > CHILD_RESOURCES_OPT_METHODS = {"RL", "RL"};


	* At this point, we need to also define the corresponding criteria (i.e., utility functions)
	* based on which the optimization is performed. Such a criterion should be performed for each one
	* of the resources to be optimized. When optimizing over Processing Bandwidth, such a criterion may
	* correspond to the processing speed, however alternative criteria may be defined.



	* Maximum allowable number of resources

	std::vector< unsigned int > MAX_NUMBER_MAIN_RESOURCES = { 2 , 1 };

	std::vector< std::vector< unsigned int > > MAX_NUMBER_CHILD_RESOURCES = { { 3, 3 }, { 0 , 0 } };


	* Initialize Scheduler
	*
	* @inputs:
	* 		- Types of Resources to be scheduled (e.g., CPU bandwidth, Memory, etc.)
	* 		- In NUMA architectures, we should be able to select node for a running thread.

	Scheduler scheduler;
	scheduler.initialize(
			   thread_count
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



	* Defining Threads / Parallelization Pattern
	*
	* @description: This part might be different for each application and it is up to the user to define properly these threads.
	* The main important input for the PaRLSched library is to get access to the id's of the threads defined.

	thread_create( scheduler.get_tinfo() );



	 * Run Scheduler
	 *
	 * @description: This function call initiates the scheduler, which involves the pre-defined resource allocation
	 *

	scheduler.run();

	double t2 = get_current_time();
	std::cout << "Total execution time: " << t2-t1 << "\n";

	pthread_exit ( NULL );         Let threads finish


}

*/
