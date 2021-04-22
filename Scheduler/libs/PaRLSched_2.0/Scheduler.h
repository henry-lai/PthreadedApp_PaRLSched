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
 * Scheduler.h
 *
 *  Created on: Feb 10, 2016
 *      Author: Georgios C. Chasparis
 * Description: This is the PaRL-sched library that executes the core of the Scheduling policy for thread-pinning.
 */

#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <iostream>
#include <vector>
#include <stdio.h>
#include <fstream>
#include <papi.h>
#include <math.h>
#include <map>
#include <numa.h>

#include "ThreadInfo.h"
#include "MethodsEstimate.h"
#include "MethodsPerformanceMonitoring.h"
#include "MethodsActions.h"
#include "MethodsOptimize.h"

#define handle_error(msg) \
        do { perror(msg); exit(EXIT_FAILURE); } while (0)

class Scheduler
{
public:

	Scheduler();
	~Scheduler();

	/*
	 * Initialize Scheduler
	 */
	void initialize(
			  const unsigned int& num_cpus
			, const unsigned int& num_threads
			, const std::vector< std::string >& RESOURCES
			, const std::vector< std::string >& RESOURCES_EST_METHODS
			, const std::vector< std::string >& RESOURCES_OPT_METHODS
			, const std::vector< unsigned int>& MAX_NUMBER_MAIN_RESOURCES
			, const std::vector< std::string >& CHILD_RESOURCES
			, const std::vector< std::string >& CHILD_RESOURCES_EST_METHODS
			, const std::vector< std::string >& CHILD_RESOURCES_OPT_METHODS
			, const std::vector< std::vector < unsigned int > >& MAX_NUMBER_CHILD_RESOURCES
			, const double& step_size
			, const double& LAMBDA
			, const double& sched_period
			, const bool& suspend_threads
			, const bool & printout_strategies
			, const bool & printout_actions
			, const bool& write_to_files
			, const bool& write_to_files_details
			, const double& gamma
			, const bool& OS_mapping
			, const bool& RL_mapping);

	/*
	 * Update Scheduler
	 */
	void update(const unsigned int& resource_ind, std::vector< bool >& update_inds, std::vector< bool >& active_threads_vec);

	/*
	 * Run Scheduler
	 */
	void run();

	/*
	 * Retrieve Performances
	 */
	void retrieve_performances(const unsigned int& resource_ind);

	/*
	 * Performance Pre-processing
	 */
	void performance_preprocessing(const unsigned int& resource_ind);

	/*
	 * Applying Scheduling Policy
	 */
	void apply_scheduling_policy();

	/*
	 * Retrieving Information from the Scheduler
	 */
	inline thread_info* get_tinfo(void)
	{
		return tinfo_;
	}


private:


	/*
	 *
	 */
	void assign_processing_node(const unsigned int& i,const unsigned int& new_numa_node, const unsigned int& new_cpu_node);

	/*
	 * Perform estimation (or formulate beliefs) over potentially beneficial allocations
	 */
	void estimate(const unsigned int& resource_ind);

	/*
	 * Perform optimization over allocations
	 */
	void optimize(const unsigned int& resource_ind);

	/*
	 * Initialize Estimates and Performance Monitoring
	 */
	void initialize_estimates(
			  const std::vector< std::string >& RESOURCES
			, const std::vector< std::string >& CHILD_RESOURCES);

	void initialize_performancemonitoring(
			  const std::vector< std::string >& RESOURCES
			, const std::vector< std::string >& CHILD_RESOURCES);

	void initialize_actions(
			  const std::vector< std::string >& RESOURCES
			, const std::vector< std::string >& CHILD_RESOURCES);


	/*
	 * Variables related to the resources optimized
	 */
	std::vector< std::string > RESOURCES_;
	std::vector< std::string > CHILD_RESOURCES_;
	std::vector< std::string > RESOURCES_OPT_METHODS_;
	std::vector< std::string > CHILD_RESOURCES_OPT_METHODS_;
	std::vector< std::string > RESOURCES_EST_METHODS_;
	std::vector< std::string > CHILD_RESOURCES_EST_METHODS_;
	std::vector< unsigned int > MAX_NUMBER_MAIN_RESOURCES_;
	std::vector< std::vector<unsigned int>> MAX_NUMBER_CHILD_RESOURCES_;

	/*
	 * Variables related to the architecture
	 */
	bool support_numa_;
	unsigned int num_threads_;
	bool suspend_threads_;

	/*
	 * Variables related to the RL algorithm for CPU pinning
	 */
	double time_;
	double time_before_;
	unsigned int sched_iteration_ = 0;
	double step_size_;
	double LAMBDA_;
	double gamma_;


	/*
	 * Scheduling period
	 */
	struct timespec ts_;
	struct timespec ts_numa_;
	struct timespec set_scheduling_period(const double& sched_period)
	{
		struct timespec ts;
		ts.tv_sec = floor(sched_period / 1e+9);
		if ( sched_period - 1e+9 * (double) ts.tv_sec > 0)
			ts.tv_nsec = sched_period - 1e+9 * (double)(ts.tv_sec);
		else
			ts.tv_nsec = sched_period;
		return ts;
	};

	/*
	 * Active Threads
	 */
	bool active_threads_;
	std::vector<bool> vec_active_threads_;

	/*
	 * Variables related to printout of strategies
	 */
	bool printout_strategies_;
	bool printout_actions_;
	bool write_to_files_;
	bool write_to_files_details_;


	/*
	 * (Performance) Estimates / Monitoring
	 * @description: We wish to create/generate estimates over the expected performance of certain resources
	 * (which may include child resources)
	 * At the same time, we wish to also observe the performance of the threads w.r.t. each one of the available resources.
	 * In the following maps, the first element refers to the 'thread_id'.
	 */
	std::map< unsigned int, std::vector< Struct_Estimate > > map_Estimate_per_Thread_;
	std::map< unsigned int, std::vector< Struct_PerformanceMonitoring> > map_PerformanceMonitoring_per_Thread_;
	std::map< unsigned int, std::vector< Struct_Actions > > map_Actions_per_Thread_;

	Struct_OverallPerformance overall_Performance_;
	Struct_MethodsOptimize methods_optimize_;
	Struct_MethodsEstimate methods_estimate_;


	/*
	 *	All information related to a thread
	 */
	thread_info * tinfo_;

	/*
	 * Variables related to the Performances of threads
	 */
	std::vector< double > vec_performances_;
	std::vector< double > vec_balanced_performances_;
	std::vector<bool> vec_performances_update_inds_;
	double cur_average_performance_;			// current average performance
	double cur_balanced_performance_;			// current balanced performance
	double run_average_performance_;			// running average performance
	double run_average_balanced_performance_;	// running average balanced performance
	std::vector< double > vec_run_average_performances_;
	std::vector< std::vector < double > > map_run_average_performances_cpu_;
	std::vector< std::vector < double > > map_run_average_balanced_performances_cpu_;
	std::vector< double > vec_min_performances_;
	std::vector< double > vec_alg_performances_;


	/*
	 * Variables related to the Active Threads
	 */
	unsigned int num_active_threads_;
	unsigned int num_active_threads_before_;

	/*
	 * Variables related to the Architecture
	 */
	unsigned int max_num_numa_nodes_;
	unsigned int max_num_cpus_;
	std::vector < std::vector< unsigned int > > cpu_nodes_per_numa_node_;

	void set_cpu_nodes_per_numa_node()
	{
		cpu_nodes_per_numa_node_.clear();
		for ( unsigned int nn=0; nn < max_num_numa_nodes_; nn++ )
		{
			std::vector < unsigned int > empty_vector;
			cpu_nodes_per_numa_node_.push_back(empty_vector);
		}
		for ( unsigned int cpu=0; cpu < max_num_cpus_; cpu++ )
		{
			cpu_nodes_per_numa_node_[numa_node_of_cpu(cpu)].push_back(cpu);
		}
	};


	/*
	 * Variables related to Streaming of Outputs/Policies
	 */
	std::fstream actionsfile_,
				avespeedfile_,
				avebalancedspeedfile_,
				timefile_,
				aveperformancefile1_,
				aveperformancefile2_,
				aveperformancefile3_,
				aveperformancefile4_,
				strategiesfile1_,
				strategiesfile2_,
				strategiesfile3_,
				strategiesfile4_;

};


#endif /* SCHEDULER_H_ */
