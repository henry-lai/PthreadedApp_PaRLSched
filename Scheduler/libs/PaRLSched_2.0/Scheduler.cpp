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

/*
 * Scheduler.cpp
 *
 *  Created on: Feb 10, 2016
 *      Author: Georgios Chasparis
 *  Updated on: Dec 09, 2016
 */

#include "Scheduler.h"
#include <math.h>
#include <sys/syscall.h>
#include <ctype.h>
#include <ctime>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <cfloat>
#include <vector>
#include <iostream>
#include <numa.h>
#include <numaif.h>
#include <errno.h>
#include <cerrno>
#include <cstring>
#include <sched.h>
#include <hwloc.h>
#include <hwloc/cpuset.h>
#include <numaif.h>

#include "ThreadControl.h"
#include "ThreadSuspendControl.h"



Scheduler::~Scheduler(void)
{
	avespeedfile_.close();
	avebalancedspeedfile_.close();
	timefile_.close();
	actionsfile_.close();
	if (write_to_files_details_)
	{
		aveperformancefile1_.close();
		aveperformancefile2_.close();
		aveperformancefile3_.close();
		aveperformancefile4_.close();
		strategiesfile1_.close();
		strategiesfile2_.close();
		strategiesfile3_.close();
		strategiesfile4_.close();
	}
};

Scheduler::Scheduler(void)
{
	num_threads_ 						= 1;
	// num_cpus_ 							= 1;
	step_size_ 							= 0.01;
	LAMBDA_ 							= 0.3;

	write_to_files_ 					= 0;
	write_to_files_details_				= 0;
	gamma_ 								= 0.02;

	cur_average_performance_ 			= 0;
	cur_balanced_performance_ 			= 0;
	num_active_threads_					= 0;
	num_active_threads_before_ 			= 0;

	printout_actions_ 					= 0;
	printout_strategies_				= 0;

	run_average_performance_ 			= 0;
	run_average_balanced_performance_ 	= 0;

	time_ 								= 0;
	time_before_ 						= 0;

	RESOURCES_							= {"NULL"};
	CHILD_RESOURCES_					= {"NULL"};
	RESOURCES_OPT_METHODS_				= {"NULL"};
	CHILD_RESOURCES_OPT_METHODS_		= {"NULL"};
	RESOURCES_EST_METHODS_				= {"NULL"};
	CHILD_RESOURCES_EST_METHODS_		= {"NULL"};
};



void Scheduler::initialize(
		  const unsigned int& num_cpus
		, const unsigned int& num_threads
		, const std::vector< std::string>& RESOURCES
		, const std::vector< std::string>& RESOURCES_EST_METHODS
		, const std::vector< std::string>& RESOURCES_OPT_METHODS
		, const std::vector< unsigned int >& MAX_NUMBER_MAIN_RESOURCES
		, const std::vector< std::string>& CHILD_RESOURCES
		, const std::vector< std::string>& CHILD_RESOURCES_EST_METHODS
		, const std::vector< std::string>& CHILD_RESOURCES_OPT_METHODS
		, const std::vector< std::vector<unsigned int >>& MAX_NUMBER_CHILD_RESOURCES
		, const double& step_size
		, const double& LAMBDA
		, const double& sched_period
		, const bool& suspend_threads
		, const bool& printout_strategies
		, const bool& printout_actions
		, const bool& write_to_files
		, const bool& write_to_files_details
		, const double& gamma
		, const bool & OS_mapping
		, const bool & RL_mapping)
{

	/*
	 * NUMA API: Tests
	 */
	if(numa_available() < 0)
	{
		printf("Your system does not support NUMA API\n");
		exit(1);
	}
	printf("NUMA Nodes available = %d\n",(int)numa_max_node()+1);

	std::cout << "NUMA distance between nodes " << numa_distance(0,1) << std::endl;
	std::cout << "NUMA number of nodes " << numa_num_configured_nodes() << std::endl;
	std::cout << "NUMA number of CPU nodes " << numa_num_configured_cpus() << std::endl;
	std::cout << "Get memory bind " << numa_get_membind() << std::endl;
	std::cout << "NUMA node of CPU 9 is  " << numa_node_of_cpu(9) << std::endl;
	std::cout << "NUMA node of CPU 11 is " << numa_node_of_cpu(11) << std::endl;
	std::cout << "NUMA node preferred of current task " << numa_preferred() << std::endl;


	/*
	 * Initializing Variables related to the Architecture
	 */
	max_num_numa_nodes_ = (unsigned int)numa_max_node()+1;
	max_num_cpus_ = (unsigned int)numa_num_configured_cpus();
	set_cpu_nodes_per_numa_node();

	/*
	 * Initialize the PAPI library
	 */
	int retval = PAPI_library_init(PAPI_VER_CURRENT);
	if (retval != PAPI_VER_CURRENT)
	{
		fprintf(stderr, "PAPI library initialization error!\n");
	}

	/*
	 * Setting variables related to the optimized resources
	 */
	RESOURCES_ = RESOURCES;
	CHILD_RESOURCES_ = CHILD_RESOURCES;
	RESOURCES_OPT_METHODS_ = RESOURCES_OPT_METHODS;
	CHILD_RESOURCES_OPT_METHODS_ = CHILD_RESOURCES_OPT_METHODS;
	RESOURCES_EST_METHODS_ = RESOURCES_EST_METHODS;
	CHILD_RESOURCES_EST_METHODS_ = CHILD_RESOURCES_EST_METHODS;

	MAX_NUMBER_MAIN_RESOURCES_ = MAX_NUMBER_MAIN_RESOURCES;
	MAX_NUMBER_CHILD_RESOURCES_ = MAX_NUMBER_CHILD_RESOURCES;

	/*
	 * Parameters w.r.t. the RL_mapping algorithm
	 */
	num_threads_ 			= num_threads;
	step_size_ 				= step_size;
	LAMBDA_ 				= LAMBDA;
	gamma_ 					= gamma;

	printout_strategies_ 	= printout_strategies;
	printout_actions_ 		= printout_actions;
	write_to_files_ 		= write_to_files;
	write_to_files_details_ = write_to_files_details;


	/*
	 * Allocating memory for thread information
	 */
	tinfo_ = (struct thread_info*) calloc(num_threads+1, sizeof(struct thread_info));
	if (tinfo_ == NULL)
		handle_error("calloc");

	/*
	 * Estimate Function
	 * For each resource that needs to be allocated, we need to define an Estimate over the
	 * estimated performance of a given source in the future. Special treatement is required when
	 * there also exists a CHILD_RESOURCE.
	 * We need to create structures of estimates for each one of the threads separately.
	 * Each thread is responsible for holding/updating this information.
	 */
	initialize_estimates(RESOURCES,CHILD_RESOURCES);
	initialize_performancemonitoring(RESOURCES,CHILD_RESOURCES);
	initialize_actions(RESOURCES,CHILD_RESOURCES);
	overall_Performance_.initialize(RESOURCES.size());

	/*
	 * Setting up the Scheduling Period
	 */
	ts_ = set_scheduling_period(sched_period);

	/*
	 * Initializing Performance Aggregators / Statistics
	 */
	for (unsigned int t=0; t<num_threads; t++)
	{
		vec_run_average_performances_.push_back(0);
		vec_performances_.push_back(0);
		vec_balanced_performances_.push_back(0);
		vec_min_performances_.push_back(DBL_MAX);
		vec_alg_performances_.push_back(0);

		std::vector < double > performance_per_cpu_tmp;
		for (unsigned int c=0; c<num_cpus; c++)
		{
			performance_per_cpu_tmp.push_back(0);
		}
		map_run_average_performances_cpu_.push_back(performance_per_cpu_tmp);
		map_run_average_balanced_performances_cpu_.push_back(performance_per_cpu_tmp);
	}

	run_average_performance_ = 0;
	run_average_balanced_performance_ = 0;
	cur_average_performance_ = 0;
	cur_balanced_performance_ = 0;


	/*
	 * Setting up vector of active threads
	 * @description: When the scheduler starts, we assume that all threads are active.
	 */
	for (unsigned int t = 0; t < num_threads_; t++)
	{
		vec_active_threads_.push_back(true);
	}
	active_threads_ = true;
	num_active_threads_ = num_threads_;
	num_active_threads_before_ = num_threads_;

	/*
	 * Suspend threads
	 */
	suspend_threads_ = suspend_threads;

	/*
	 * Setting CPU affinity of master thread
	 */
	cpu_set_t my_set;        /* Define your cpu_set bit mask. */
	CPU_ZERO(&my_set);       /* Initialize it all to 0, i.e. no CPUs selected. */
	CPU_SET(0, &my_set);
	sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	/*
	 * Initializing Strategies for Threads
	 */
	double sum_strategies(0);


	/*
	 * Initializing Vector of Performances, Performances Indices and Actions (per thread)
	 */
	for (unsigned int t = 0; t < num_threads_; t++)
	{
		vec_performances_.push_back(0);
		vec_performances_update_inds_.push_back(false);
		// actions_.push_back(0);
	}

	struct timeval tim;
	gettimeofday(&tim, NULL);
	time_before_ = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	time_ = 0;

	// Opening files for recording performances
	timefile_.open("time.txt",std::ios::out);
	avespeedfile_.open("avespeed.txt",std::ios::out);
	avebalancedspeedfile_.open("avebalancedspeedfile.txt",std::ios::out);
	actionsfile_.open("actionsfile.txt",std::ios::out);

	if (write_to_files_details_)
	{
		aveperformancefile1_.open("aveperformance1.txt",std::ios::out);
		aveperformancefile2_.open("aveperformance2.txt",std::ios::out);
		aveperformancefile3_.open("aveperformance3.txt",std::ios::out);
		aveperformancefile4_.open("aveperformance4.txt",std::ios::out);
		strategiesfile1_.open("strategies1.txt",std::ios::out);
		strategiesfile2_.open("strategies2.txt",std::ios::out);
		strategiesfile3_.open("strategies3.txt",std::ios::out);
		strategiesfile4_.open("strategies4.txt",std::ios::out);
	}

};




void Scheduler::run()
{

    // Running main control loop
	while (active_threads_)
	{

		std::cout << " ~~~~~~~~~~~~~~ new scheduler update ~~~~~~~~~~~~~~~\n";

		/*
		 * We would like the scheduler to
		 */
		nanosleep(&ts_, NULL);
		sched_iteration_++;
		std::cout << " sched iteration " << sched_iteration_ << std::endl;

		active_threads_ = false;
		std::cout << " The current thread runs on CPU: " << sched_getcpu() << std::endl;

		/*
		 * Performance Counting and Scheduling Update
		 */
		for (unsigned int r=0; r<1; r++)
			retrieve_performances(r);

		/*
		 * Performance Pre-processing
		 * This is performed for each one of the resources to be optimized
		 */
		for (unsigned int r = 0; r < 1; r++)
			performance_preprocessing(r);

		if (active_threads_)
		{
			/*
			 * update
			 * @description: the update function of the scheduler should be performed for each one of the main RESOURCES
			 */
			for (unsigned int r = 0; r < 1; r++){
				update( r, vec_performances_update_inds_, vec_active_threads_ );
			}

		}

		/*
		 * Applying Scheduling Policy
		 */
		apply_scheduling_policy();

	}

}


/*
 * Scheduler::update
 * @description: the main purpose of the update function of the scheduler is to perform two main functionalities
 * a) ESTIMATE (compute estimates of the performance of each one of the threads (given the resource over which optimization is perfomed)
 * b) OPTIMIZE (optimize allocations based on prior estimates or independently of such estimates)
 * It is implicitly assumed here, that alternative methodologies may be developed.
 */

void Scheduler::update(const unsigned int& resource_ind, std::vector<bool>& update_inds, std::vector< bool >& active_threads_vec)
{


	/*
	 * ESTIMATION
	 * Note that it makes sense to update the estimates of a thread only for the main source that is is used by the thread.
	 */
	estimate(resource_ind);


	/*
	 * OPTIMIZATION
	 */
	optimize(resource_ind);


	/*
	 * We update the number of active threads
	 * */
	num_active_threads_before_ = num_active_threads_;


	/*
	 * Writing to files
	 * */
	if (write_to_files_)
	{

		// std::cout << " status " << exist_updated_threads << " , iteration " << sched_iteration_ << std::endl;

		// writing action to files:
		avespeedfile_ << run_average_performance_ << "\n";
		avebalancedspeedfile_ << run_average_balanced_performance_ << "\n";
		timefile_ << time_ << "\n";
		for (unsigned int t=0;t<num_threads_;t++)
		{
			std::map<unsigned int, std::vector<Struct_Actions>>::iterator map_actions_it = map_Actions_per_Thread_.find(t);
			actionsfile_ << map_actions_it->second[0].vec_actions_per_child_source_[0] << ";";
			if (t==num_threads_-1){
				actionsfile_ << std::endl;
			}
		}
		if (write_to_files_details_){
			aveperformancefile1_ << vec_run_average_performances_[0] << "\n";
			aveperformancefile2_ << vec_run_average_performances_[1] << "\n";
			aveperformancefile3_ << vec_run_average_performances_[2] << "\n";
			aveperformancefile4_ << vec_run_average_performances_[3] << "\n";
//			strategiesfile1_ << map_strategies_[0][0] << ";" << map_strategies_[0][1] << ";" << map_strategies_[0][2] << ";" << map_strategies_[0][3] << "\n";
//			strategiesfile2_ << map_strategies_[1][0] << ";" << map_strategies_[1][1] << ";" << map_strategies_[1][2] << ";" << map_strategies_[1][3] << "\n";
//			strategiesfile3_ << map_strategies_[2][0] << ";" << map_strategies_[2][1] << ";" << map_strategies_[2][2] << ";" << map_strategies_[2][3] << "\n";
//			strategiesfile4_ << map_strategies_[3][0] << ";" << map_strategies_[3][1] << ";" << map_strategies_[3][2] << ";" << map_strategies_[3][3] << "\n";
		}
	}
}


/*
 * estimate
 */
void Scheduler::estimate(const unsigned int& resource_ind)
{

	if (printout_strategies_)
	{
		std::cout << "~~~~~Strategies\n ";
	}

	std::map<unsigned int, std::vector< Struct_Actions > >::iterator it_map_actions = map_Actions_per_Thread_.begin();
	std::map<unsigned int, std::vector< Struct_Estimate > >::iterator it_map_estimates;
	std::map<unsigned int, std::vector< Struct_PerformanceMonitoring > >::iterator it_map_performances;

	// temporarily, we will first reproduce all the strategy updates below in the form of estimates
	// we perform the following loop over threads
	for (it_map_actions; it_map_actions!=map_Actions_per_Thread_.end(); ++it_map_actions)
	{
		it_map_estimates = map_Estimate_per_Thread_.find(it_map_actions->first);
		it_map_performances = map_PerformanceMonitoring_per_Thread_.find(it_map_actions->first);

		if (vec_active_threads_[it_map_actions->first] == false)
			continue;		// we only update the strategies when there is an update in the performance


		// for this thread, we get the structure of Estimates over the child resources
		unsigned int action_main = it_map_actions->second[resource_ind].action_per_main_source_;
		unsigned int action_child = it_map_actions->second[resource_ind].vec_actions_per_child_source_[action_main];
		double cur_balanced_performance = it_map_performances->second[resource_ind].balanced_performance_;

		std::vector<double>& main_estimates = it_map_estimates->second[resource_ind].vec_estimates_;
		std::vector<double>& main_cummulative_estimates = it_map_estimates->second[resource_ind].vec_cummulative_estimates_;
		std::vector<double>& child_estimates = it_map_estimates->second[resource_ind].vec_child_estimates_[action_main].vec_estimates_;
		std::vector<double>& child_cummulative_estimates = it_map_estimates->second[resource_ind].vec_child_estimates_[action_main].vec_cummulative_estimates_;

		// Updating Estimates over the Main Resource
		if (RESOURCES_EST_METHODS_[resource_ind].compare("RL")==0){
			methods_estimate_.RL_update(main_estimates, main_cummulative_estimates, cur_balanced_performance, action_main, step_size_);

		}

		// Updating Estimates over the Child Resources
		if (CHILD_RESOURCES_EST_METHODS_[resource_ind].compare("RL")==0){
			//std::cout << " prior child action " << action_child << std::endl;
			//std::cout << " prior strategy " << child_estimates[action_child] << std::endl;
			methods_estimate_.RL_update(child_estimates, child_cummulative_estimates, cur_balanced_performance, action_child, step_size_);
			//std::cout << " new strategy " << child_estimates[action_child] << std::endl;
		}

		if (printout_strategies_){

			std::cout << "  - thread " << it_map_actions->first << " -- \n";
			std::cout << "  - NUMA strategies \n";
			for (unsigned int n = 0; n < main_estimates.size(); n++){
				std::cout << "      numa node " << n << " = " << main_estimates[n] << std::endl;
			}
			std::cout << "  - CPU strategies for selected NUMA node : " << action_main << std::endl;
			for (unsigned int s = 0; s < child_estimates.size(); s++){
				std::cout << "      cpu " << s << " = " << child_estimates[s] << std::endl;
			}
		}

	}
}


/*
 * optimize
 *
 * @description: This function updates the allocations of the threads w.r.t. the optimized resources
 * For example, in the case of the RL algorithm, the estimates in the estimate function are directly
 * used to generate allocations. Alternative strategies may be employed. For example, one strategy could
 * be to play instead the action with the largest performance so far, and so on.
 */
void Scheduler::optimize(const unsigned int& resource_ind)
{

	if (printout_actions_)
		std::cout << "~~~~~Actions selected -- \n";

	// the main goal here is to define a new action profile over the selected resources.
	std::map<unsigned int, std::vector< Struct_Actions > >::iterator it_map_actions = map_Actions_per_Thread_.begin();
	std::map<unsigned int, std::vector< Struct_Estimate > >::iterator it_map_estimates;
	std::map<unsigned int, std::vector< Struct_PerformanceMonitoring > >::iterator it_map_performances;

	for (it_map_actions; it_map_actions!=map_Actions_per_Thread_.end(); ++it_map_actions)
	{
		// for each one of the (running) threads
		it_map_estimates = map_Estimate_per_Thread_.find(it_map_actions->first);
		it_map_performances = map_PerformanceMonitoring_per_Thread_.find(it_map_actions->first);

		// we only update the actions for the active threads
		if (vec_active_threads_[it_map_actions->first] == false)
			continue;

		if (RESOURCES_OPT_METHODS_[resource_ind].compare("RL") == 0)
			methods_optimize_.RL_optimize(it_map_performances->second[resource_ind], it_map_estimates->second[resource_ind], it_map_actions->second[resource_ind], LAMBDA_);

		// new actions
		unsigned int action_main = it_map_actions->second[resource_ind].action_per_main_source_;
		unsigned int action_child = it_map_actions->second[resource_ind].vec_actions_per_child_source_[action_main];

	}

}

/*
 * initialize_estimates()
 */
void Scheduler::initialize_estimates(const std::vector<std::string>& RESOURCES, const std::vector<std::string>& CHILD_RESOURCES)
{

	for (unsigned int i = 0; i < num_threads_; i++){
		std::vector< Struct_Estimate > vec_Estimates_per_Thread;
		for (unsigned int r = 0; r < RESOURCES.size(); r++){
			// we create an estimate per resource
			Struct_Estimate estimate;
			if (RESOURCES[r].compare("NUMA_PROCESSING")==0){
				// in case the resource to be allocated corresponds to the NUMA_PROCESSING, then we need to also check if there is
				// a corresponding child resource, that is "CPU_PROCESSING"
				// we first need to retrieve the maximum number of NUMA nodes available

				if (CHILD_RESOURCES[r].compare("CPU_PROCESSING")==0)
					// for each one of the numa nodes, we need to get the maximum number of CPU nodes
					estimate.initialize_w_child ( RESOURCES[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r], CHILD_RESOURCES[r], cpu_nodes_per_numa_node_, MAX_NUMBER_CHILD_RESOURCES_[r] );
				else
					estimate.initialize(RESOURCES[r],max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r]);
			}
			else if ( RESOURCES[r].compare("NUMA_MEMORY")==0 )
				estimate.initialize( RESOURCES[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r]);
			vec_Estimates_per_Thread.push_back ( estimate );
		}
		map_Estimate_per_Thread_.insert(std::pair< unsigned int, std::vector< Struct_Estimate > > ( i, vec_Estimates_per_Thread));
	}
}


/*
 * initialize_performancemonitoring()
 * @description: The function admits a similar structure to the initialize_estimates() function and initializes all the necessary
 * variables for keeping track of the performance of the threads with respect to the resources to be optimized.
 */
void Scheduler::initialize_performancemonitoring(const std::vector<std::string>& RESOURCES, const std::vector<std::string>& CHILD_RESOURCES)
{
	for (unsigned int i = 0; i < num_threads_; i++){
		std::vector < Struct_PerformanceMonitoring > vec_Performances_per_Thread;
		for (unsigned int r = 0; r < RESOURCES.size(); r++){
			// we create a PerformanceMonitoring structure per resource optimized
			Struct_PerformanceMonitoring performancemonitoring;
			if (RESOURCES[r].compare("NUMA_PROCESSING") == 0){
				if (CHILD_RESOURCES[r].compare("CPU_PROCESSING") == 0)
					performancemonitoring.initialize_w_child( RESOURCES[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r], CHILD_RESOURCES[r], cpu_nodes_per_numa_node_, MAX_NUMBER_CHILD_RESOURCES_[r]  );
				else
					performancemonitoring.initialize(RESOURCES[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r]);
			}
			else if (RESOURCES[r].compare("NUMA_MEMORY") == 0)
				performancemonitoring.initialize(RESOURCES[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r]);
			vec_Performances_per_Thread.push_back( performancemonitoring );
		}
		map_PerformanceMonitoring_per_Thread_.insert(std::pair< unsigned int, std::vector< Struct_PerformanceMonitoring > >( i , vec_Performances_per_Thread));
	}
}


/*
 * initialize_actions()
 * @description: The function admits a similar structure to the initialize_estimates() function and initializes all the necessary
 * variables for keeping track of the actions of the threads with respect to the optimized resources
 */
void Scheduler::initialize_actions(
			  const std::vector< std::string >& RESOURCES
			, const std::vector< std::string >& CHILD_RESOURCES
			)
{
	for (unsigned int i = 0; i < num_threads_; i++){
		std::vector< Struct_Actions > vec_Actions_per_Thread;
		for (unsigned int r = 0; r < RESOURCES.size(); r++){
			Struct_Actions actions;
			if (RESOURCES[r].compare("NUMA_PROCESSING") == 0){
				if (CHILD_RESOURCES[r].compare("CPU_PROCESSING") == 0)
					actions.initialize_w_child ( RESOURCES[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r], CHILD_RESOURCES[r], cpu_nodes_per_numa_node_, MAX_NUMBER_CHILD_RESOURCES_[r] );
				else
					actions.initialize(RESOURCES[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r]);
			}
			else if (RESOURCES[r].compare("NUMA_MEMORY") == 0)
				actions.initialize(RESOURCES[r], max_num_numa_nodes_, MAX_NUMBER_MAIN_RESOURCES_[r]);
			vec_Actions_per_Thread.push_back ( actions );
		}
		map_Actions_per_Thread_.insert(std::pair< unsigned int, std::vector< Struct_Actions > > (i , vec_Actions_per_Thread));
	}
}


/*
 * retrieve_performances()
 * @description: this function retrieves the  performances for each one of the threads and for each one of the main resources
 */
void Scheduler::retrieve_performances(const unsigned int& resource_ind)
{

	/*
	 * For now, let us assume that we one performance counter for each one of the main resources.
	 * In other words, for now the resource_ind does not have any impact.
	 *
	 * However, we need to find an interface for thd_record_counters function, to accept the main resource as an input.
	 */
	if (printout_strategies_)
	{
		std::cout << "~~~~~Performances\n";
	}


	ThreadControl thread_control;

	std::map<unsigned int, std::vector<Struct_PerformanceMonitoring>>::iterator it_map_performances = map_PerformanceMonitoring_per_Thread_.begin();
	for ( it_map_performances; it_map_performances!=map_PerformanceMonitoring_per_Thread_.end(); ++it_map_performances )
	{
		// for each one of the threads
		unsigned int thread_counter = it_map_performances->first;
		//std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
		// std::cout << " Retrieving measurements for thread " << it_map_performances->first << std::endl;
		if (!thread_control.thd_record_counters(tinfo_[thread_counter].thread_id,&tinfo_[thread_counter]))
			printf("Error: Problem recording counters for thread %d", (int)tinfo_[thread_counter].thread_id);
		it_map_performances->second[resource_ind].performance_ = tinfo_[thread_counter].performance / 1e+8;
		it_map_performances->second[resource_ind].performance_update_ind_ = tinfo_[thread_counter].performance_update_ind;
		if (tinfo_[thread_counter].status == 0){
			vec_active_threads_[thread_counter] = true;	// the thread has not completed its task.
			active_threads_ = true;
		}
		else
			vec_active_threads_[thread_counter] = false;
	}

}


void Scheduler::performance_preprocessing(const unsigned int& resource_ind)
{
	/*
	 * Performance Pre-Processing
	 * Description: The purpose of this function is to perform necessary pre-processing of the available performances
	 * (e.g., computing the average, the running average, etc.)
	 *
	 */

	double sum_performances = 0;
	double ave_performance = 0;
	//double sum_squared_performances = 0;
	double sum_balanced_performances = 0;
	double ave_balanced_performance = 0;


	struct timeval tim;
	gettimeofday(&tim, NULL);

	double current_time = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	time_ = time_ + ( current_time - time_before_);
	time_before_ = current_time;

	num_active_threads_ = 0;

	/*
	 * Computing the Sum of Performances
	 */
	std::map< unsigned int, std::vector< Struct_PerformanceMonitoring > >::iterator it_performance = map_PerformanceMonitoring_per_Thread_.begin();
	std::map< unsigned int, std::vector< Struct_Actions > >::iterator map_Actions_it;

	for (it_performance; it_performance != map_PerformanceMonitoring_per_Thread_.end(); ++it_performance)
	{
		map_Actions_it = map_Actions_per_Thread_.find(it_performance->first);

		if (vec_active_threads_[it_performance->first] == true)
		{
			num_active_threads_ += 1;
			// I update the performances, although for some might be the same.
			// The idea here is that if the thread is active we take into account its performance
			// for strategy update.
			// vec_performances_[t] = vec_performances_[t]/1e+8;
			sum_performances += it_performance->second[resource_ind].performance_;
			// updating the running average performance
			it_performance->second[resource_ind].update_run_average_performance(it_performance->second[resource_ind].performance_,step_size_);
			// updating the running average performance per main resource
			it_performance->second[resource_ind].update_run_average_performance_per_main_resource(it_performance->second[resource_ind].performance_,map_Actions_it->second[resource_ind].action_per_main_source_,step_size_);
			// updating the running average performance per child resource
			it_performance->second[resource_ind].update_run_average_performance_per_child_resource(it_performance->second[resource_ind].performance_,map_Actions_it->second[resource_ind].action_per_main_source_,
					map_Actions_it->second[resource_ind].vec_actions_per_child_source_[resource_ind],step_size_);
		}
	}

	// average performance
	if (num_active_threads_>0)
		ave_performance = sum_performances / (double)num_active_threads_;
	else
		ave_performance = sum_performances;

	// balanced performance
	it_performance = map_PerformanceMonitoring_per_Thread_.begin();
	for (it_performance; it_performance != map_PerformanceMonitoring_per_Thread_.end(); ++it_performance)
	{
		// map_Actions_it = map_Actions_per_Thread_.find(it_performance->first);

		if (vec_active_threads_[it_performance->first] == true)
		{
			// I update the performances, although for some might be the same.
			// The idea here is that if the thread is active we take into account its performance
			// for strategy update.
			// vec_performances_[t] = vec_performances_[t]/1e+8;
			it_performance->second[resource_ind].balanced_performance_ = it_performance->second[resource_ind].performance_ - gamma_ * pow(it_performance->second[resource_ind].performance_ - ave_performance, 2);
			sum_balanced_performances += it_performance->second[resource_ind].performance_ - gamma_ * pow(it_performance->second[resource_ind].performance_ - ave_performance, 2);
			// updating the running average balanced performance
			it_performance->second[resource_ind].update_run_average_balanced_performance(it_performance->second[resource_ind].balanced_performance_,step_size_);
		}
	}

	ave_balanced_performance = sum_balanced_performances/(double)num_active_threads_;

	overall_Performance_.update_sum_performances_per_main_resource(resource_ind, sum_performances);
	overall_Performance_.update_ave_performance_per_main_resource(resource_ind,ave_performance);

	overall_Performance_.update_sum_balanced_performances_per_main_resource(resource_ind, sum_balanced_performances);
	overall_Performance_.update_ave_balanced_performance_per_main_resource(resource_ind, ave_balanced_performance);

	overall_Performance_.update_run_average_performance_per_main_resource(resource_ind, ave_performance, sched_iteration_);
	overall_Performance_.update_run_average_balanced_performance_per_main_resource(resource_ind, ave_balanced_performance, sched_iteration_);


	std::cout << "~~~~~Average Performances \n";
	std::cout << "  current average performance : " << ave_performance << ", run average perf. " << overall_Performance_.run_average_performance_[resource_ind] << std::endl;
	std::cout << "  current balanced performance : " << ave_balanced_performance << ", run average balanced perf. " << overall_Performance_.run_average_balanced_performance_[resource_ind] << std::endl;

}


void Scheduler::apply_scheduling_policy()
{

	/*
	 * This function applies the scheduling action derived from optimize()
	 *
	 * a) Assigning main resource (e.g., NUMA node for processing)
	 * b) Assigning child resource (e.g., CPU node for processing)
	 */
	for (unsigned int i = 0; i < num_threads_; i++){
		if ( tinfo_[i].status == 0 ){
			/*
			 * Suspending thread before setting its affinity
			 */
			if (suspend_threads_){
				printf ("Suspending thread %d.\n", i);
				if (thd_suspend (tinfo_[i].thread_id) != 0){
					printf ("%s:%d\t ERROR: Suspend thread failed!\n", __FILE__, __LINE__);
				}
			}

			/*
			 * Setting its CPU affinity
			 */
			unsigned int new_main_action;
			unsigned int new_numa_node;
			unsigned int new_child_action;
			unsigned int new_cpu_node;



			for (unsigned int r = 0; r < RESOURCES_.size(); r++){
				// for each one of the main resources
				std::map<unsigned int, std::vector< Struct_Actions> >::iterator it_action = map_Actions_per_Thread_.find(i);

				if ((RESOURCES_[r].compare("NUMA_PROCESSING") == 0) && (CHILD_RESOURCES_[r].compare("CPU_PROCESSING") == 0))
				{

					// we perform all necessary actions for assigning the new NUMA node
					// the action that needs to be implemented is:
					new_main_action = it_action->second[r].action_per_main_source_;
					new_numa_node = new_main_action;
					new_child_action = it_action->second[r].vec_actions_per_child_source_[r];
					new_cpu_node = it_action->second[r].vec_child_sources_[new_main_action][new_child_action];

					assign_processing_node(i,new_numa_node,new_cpu_node);

				}

			}

			/*
			 * Continuing running thread after setting its affinity
			 */
			if ( suspend_threads_ ){
				printf ("Continuing thread %d.\n", i);
				if (thd_continue (tinfo_[i].thread_id) != 0){
					printf ("%s:%d\t ERROR: Continuing thread failed!\n", __FILE__, __LINE__);
				}
			}

		}
		else{
			std::cout << " Status of thread " << i << ": FINISHED!" << " ( time = " << tinfo_[i].termination_time << " )" << std::endl;
		}

	} // end of applying scheduling policy
}


/*
 * Scheduler::assign_processing_node
 */
void Scheduler::assign_processing_node(const unsigned int& thread, const unsigned int& numa_node, const unsigned int& cpu_node)
{


	cpu_set_t mask;
	CPU_ZERO (&mask);
	CPU_SET( cpu_node , &mask);
	std::cout << " thread " << tinfo_[thread].thread_num << " will run on CPU " << cpu_node << std::endl;

	/*
	 * bind process to processor
	 * */
	if (pthread_setaffinity_np(tinfo_[thread].thread_id, sizeof(mask), &mask) <0)
	{
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
	}

	/*
	 * Getting the current CPU affinity
	 */
	cpu_set_t mask_tmp;
	int output_get_cpu_affinity = pthread_getaffinity_np(tinfo_[thread].thread_id, sizeof(mask_tmp), &mask_tmp);
	std::vector<int> vec_cpus;
	for (int cpu=0;cpu<20;cpu++){
		if (CPU_ISSET(cpu,&mask_tmp) != 0)
		{
			vec_cpus.push_back(cpu);
			// std::cout << " cpu " << cpu << " is in the affinity of thread " << tinfo_[thread].thread_id << std::endl;
		}
	}

	/*
	 * We retrieve the NUMA node of the first allowed CPU
	 */
	int active_numa_node;
	if (vec_cpus.size()>0){
		active_numa_node = numa_node_of_cpu(vec_cpus[0]);
	}
}
