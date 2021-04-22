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

#include "ThreadControl.h"
#include "ThreadSuspendControl.h"


Scheduler::~Scheduler(void)
{
	avespeedfile_.close();
	avebalancedspeedfile_.close();
	timefile_.close();
	actionsfile_.close();
	if (write_to_files_details_){
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
	num_threads_ 			= 1;
	num_cpus_ 				= 1;
	step_size_ 				= 0.01;
	LAMBDA_ 				= 0.3;

	write_to_files_ 		= 0;
	write_to_files_details_	= 0;
	gamma_ 					= 0.02;

	cur_average_performance_ = 0;
	cur_balanced_performance_ = 0;
	num_active_threads_		= 0;
	num_active_threads_before_ = 0;

	printout_actions_ 		= 0;
	printout_strategies_	= 0;

	run_average_performance_ = 0;
	run_average_balanced_performance_ = 0;

	time_ = 0;
	time_before_ = 0;
};

Scheduler::Scheduler(const unsigned int& num_cpus, const unsigned int& num_threads, const double& step_size, const double& LAMBDA,
		const bool& printout_strategies, const bool& printout_actions, const bool& write_to_files, const bool& write_to_files_details, const double& gamma)
{

	// Initialize the PAPI library
	int retval = PAPI_library_init(PAPI_VER_CURRENT);
	if (retval != PAPI_VER_CURRENT) {
		fprintf(stderr, "PAPI library initialization error!\n");
		// return(1);
	}


	num_cpus_ = num_cpus;
	num_threads_ = num_threads;
	step_size_ = step_size;
	LAMBDA_ = LAMBDA;
	gamma_ = gamma;

	printout_strategies_ = printout_strategies;
	printout_actions_ = printout_actions;
	write_to_files_ = write_to_files;
	write_to_files_details_ = write_to_files_details;
	for (unsigned int t=0; t<num_threads; t++)
	{
		run_average_performances_.push_back(0);
		performances_.push_back(0);
		balanced_performances_.push_back(0);
		min_performances_.push_back(DBL_MAX);
		alg_performances_.push_back(0);

		std::vector < double > performance_per_cpu_tmp;
		for (unsigned int c=0; c<num_cpus; c++)
		{
			performance_per_cpu_tmp.push_back(0);
		}
		run_average_performances_cpu_.push_back(performance_per_cpu_tmp);
		run_average_balanced_performances_cpu_.push_back(performance_per_cpu_tmp);
	}

	run_average_performance_ = 0;
	run_average_balanced_performance_ = 0;
	cur_average_performance_ = 0;
	cur_balanced_performance_ = 0;

	/*
	 * Active Threads
	 */
	num_active_threads_ = 0;
	num_active_threads_before_ = 0;

	/*
	 * CPU Schedule Initialization
	 * */
	for (unsigned int i=0;i<num_threads;i++){
		cpu_schedule_.push_back(0);		// we assign to each thread cpu 0
	}

	/*
	 * Thread to Strategy
	 * */
	double sum_strategies(0);

	for (unsigned int i=0;i<num_threads;i++)
	{
		std::vector<double> tmp_strategies;
		std::vector<double> tmp_com_strategies;
		sum_strategies = 0;
		for (unsigned int j=0;j<num_cpus;j++){
		    tmp_strategies.push_back(1/(double)num_cpus);
			tmp_com_strategies.push_back(sum_strategies);
			sum_strategies += 1/(double)num_cpus;
		}
		map_strategies_.push_back(tmp_strategies);
		com_map_strategies_.push_back(tmp_com_strategies);
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


void Scheduler::update(std::vector<double>& performances, std::vector<int>& actions, int iteration, std::vector<bool>& update_inds,
		std::vector< bool >& active_threads_vec, const bool& RL_mapping)
{

	/**
	 * Performance Normalization
	 *
	 * */
	bool exist_updated_threads = false;

	double sum_performances = 0;
	//double sum_squared_performances = 0;
	double sum_balanced_performances = 0;

	struct timeval tim;
	gettimeofday(&tim, NULL);

	double current_time = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	time_ = time_ + ( current_time - time_before_);
	time_before_ = current_time;

	// Find which threads has been updated with performance
	std::vector<bool> updated_threads;

	// Computation of the Current Average Performance
	num_active_threads_ = 0;
	for (unsigned int t=0; t< num_threads_; t++){
		if (active_threads_vec[t] == true){
			num_active_threads_ += 1;
			// I update the performances, although for some might be the same.
			// The idea here is that if the thread is active we take into account its performance
			// for strategy update.
			performances_[t] = performances[t]/1e+8;
			sum_performances += performances_[t];
		}
		else {
			performances_[t] = 0;
			actions[t] = -1;		/* if this is not an active thread, we assign action -1 */
		}
	}


	// Computation of the current average performance:
	cur_average_performance_ = sum_performances / (double)num_active_threads_;

	std::cout << " Number of active threads = " << num_active_threads_ << std::endl;

	// Computation of the Average Balanced Performance
	sum_balanced_performances = 0;
	for (unsigned int t=0; t< num_threads_; t++)
	{
		/*
		 * Here we create a sum of balanced performances only for the active threads
		 * */
		if (active_threads_vec[t] == true){
			sum_balanced_performances += performances_[t] - gamma_ * pow(performances_[t] - cur_average_performance_, 2);
		}
	}
	cur_balanced_performance_ = sum_balanced_performances / (double)num_active_threads_;

	for (unsigned int t=0; t< num_threads_; t++){

		if (active_threads_vec[t] == true)
			run_average_performances_[t] = run_average_performances_[t] + step_size_ * (performances_[t] - run_average_performances_[t]);

		// We also compute the running average performance per cpu
		// We only update it for the cpu that was selected in the past
		for (unsigned int c=0;c < num_cpus_; c++)
		{
			if (c == cpu_schedule_[t])
				run_average_performances_cpu_[t][c] = run_average_performances_cpu_[t][c] + step_size_ * (performances_[t] - run_average_performances_cpu_[t][c]);
		}
	}

	run_average_performance_ = run_average_performance_ +
			1/(pow((double)iteration,0.7)+1) * (cur_average_performance_ - run_average_performance_);
	run_average_balanced_performance_ = run_average_balanced_performance_ +
			1/(pow((double)iteration,0.7)+1) * (cur_balanced_performance_ - run_average_balanced_performance_);

	std::cout << "  current average performance : " << cur_average_performance_ << ", run average perf. " << run_average_performance_ << std::endl;
	std::cout << "  current balanced performance : " << cur_balanced_performance_ << ", run average balanced perf. " << run_average_balanced_performance_ << std::endl;

	/*
	 * Policy update for each thread
	 * */
	//std::cout << " RL mapping " << RL_mapping << std::endl;
	if (RL_mapping ){ // && (iteration%200 >= 0 && iteration%200 < 100)
		for (unsigned int t=0;t<num_threads_;t++){
			// for each one of the threads40
			if (update_inds[t] == false)
				continue;	// we only update the strategies when there is an update in the performance

			exist_updated_threads = true;

			/*
			 * If the number of active threads have changed, we reset (normalize) the strategies of this thread
			 * */

			if (printout_strategies_)
				std::cout << "  -- Strategies for thread " << t << " -- \n";

			for (unsigned int a=0;a<num_cpus_;a++)
			{
				// for each one of the possible actions
				if (a == cpu_schedule_[t]){
					map_strategies_[t][a] = map_strategies_[t][a] + step_size_ * cur_balanced_performance_ * (1 - map_strategies_[t][a]);
					if (printout_strategies_)
						std::cout << "     cpu " << a << " = " << map_strategies_[t][a] << std::endl;
				}
				else{
					map_strategies_[t][a] = map_strategies_[t][a] + step_size_ * cur_balanced_performance_ * (0 - map_strategies_[t][a]);

					if (printout_strategies_)
						std::cout << "     cpu " << a << " = " << map_strategies_[t][a] << std::endl;
				}

			}
		}

		/*
		 * Given the updated strategies, we are computing the commulative strategy matrix
		 */
		for (unsigned int t=0;t<num_threads_;t++){
			for (unsigned int a=0;a<num_cpus_;a++){
				if (a==0)
					com_map_strategies_[t][a] = map_strategies_[t][a];
				else
					com_map_strategies_[t][a] = com_map_strategies_[t][a-1] + map_strategies_[t][a];
			}
		}

		/*
		 * Action Selection
		 * */
		unsigned int action;
		double tmp1;
		double tmp2;

		for (unsigned int t=0;t<num_threads_;t++){
			if (update_inds[t] == false)
				continue;

			tmp1 = rand() % 100;
			tmp2 = rand() % 100;

			if (t==0 && printout_actions_)
				std::cout << "  -- Actions selected -- \n";

			if (tmp2 <= ( LAMBDA_ / cur_balanced_performance_ ) * 100){
			// if (tmp2 <= ( LAMBDA_ / cur_average_performance_ ) * 100){
				// then, we select cpu according to a uniform distribution
				for (unsigned int i=0;i<num_cpus_;i++)
				{
					if ((i==0) && (tmp1 >=0) && (tmp1 < (i+1) * (1/(double)num_cpus_) * 100)){
						action = i;
						break;
					}
					else if ((i>0)&&(tmp1 >= i * (1/(double)num_cpus_) * 100) && (tmp1 < (i+1) * (1/(double)num_cpus_) * 100)){
						action = i;
						break;
					}
				}
			}
			else{
				for (unsigned int i=0;i<num_cpus_;i++)
				{
					if ((i==0) && (tmp1 >=0) && (tmp1 < com_map_strategies_[t][i] * 100)){
						action = i;
						break;
					}
					else if ((i>0)&&(tmp1 >= com_map_strategies_[t][i-1] * 100) && (tmp1 < com_map_strategies_[t][i] * 100)){
						action = i;
						break;
					}
				}
			}

			if (printout_actions_)
				std::cout << "thread " << t+1 << " to cpu " << action << std::endl;

			// Setting cpu schedule:
			cpu_schedule_[t] = action;
		}
	}

	/*
	 * We update the number of active threads
	 * */
	num_active_threads_before_ = num_active_threads_;

	/*
	 * Writing to files
	 * */
	if (write_to_files_){

		std::cout << " status " << exist_updated_threads << " , iteration " << iteration << std::endl;

		// writing action to files:

		avespeedfile_ << run_average_performance_ << "\n";
		avebalancedspeedfile_ << run_average_balanced_performance_ << "\n";
		timefile_ << time_ << "\n";
		for (unsigned int t=0;t<num_threads_;t++){
			actionsfile_ << actions[t] << ";";
			if (t==num_threads_-1){
				actionsfile_ << std::endl;
			}
		}
		if (write_to_files_details_){
			aveperformancefile1_ << run_average_performances_[0] << "\n";
			aveperformancefile2_ << run_average_performances_[1] << "\n";
			aveperformancefile3_ << run_average_performances_[2] << "\n";
			aveperformancefile4_ << run_average_performances_[3] << "\n";
			strategiesfile1_ << map_strategies_[0][0] << ";" << map_strategies_[0][1] << ";" << map_strategies_[0][2] << ";" << map_strategies_[0][3] << "\n";
			strategiesfile2_ << map_strategies_[1][0] << ";" << map_strategies_[1][1] << ";" << map_strategies_[1][2] << ";" << map_strategies_[1][3] << "\n";
			strategiesfile3_ << map_strategies_[2][0] << ";" << map_strategies_[2][1] << ";" << map_strategies_[2][2] << ";" << map_strategies_[2][3] << "\n";
			strategiesfile4_ << map_strategies_[3][0] << ";" << map_strategies_[3][1] << ";" << map_strategies_[3][2] << ";" << map_strategies_[3][3] << "\n";
		}
	}
}





void Scheduler::run(const unsigned int & thread_count, const unsigned int& num_cpus, const bool & OS_mapping, const bool & RL_mapping,
		const double & sched_period, const bool & suspend_threads, thread_info* tinfo)
{

	/*
	 * Recording Current Time
	 */
	struct timespec ts;

	ts.tv_sec = floor(sched_period / 1e+9);
	if ( sched_period - 1e+9 * (double) ts.tv_sec > 0)
		ts.tv_nsec = sched_period - 1e+9 * (double)(ts.tv_sec);
	else
		ts.tv_nsec = sched_period;


	/*
	 * Setting CPU affinity of master thread
	 */
    cpu_set_t my_set;        /* Define your cpu_set bit mask. */
    CPU_ZERO(&my_set);       /* Initialize it all to 0, i.e. no CPUs selected. */
    CPU_SET(0, &my_set);
    sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	/*
	 * Defining vector of active threads
	 */
	std::vector<bool> active_threads_vec;
	for (unsigned int t=0;t<thread_count;t++){
		active_threads_vec.push_back(true);
	}
	bool active_threads(true);

	/*
	 * Running Scheduling Loop
	 */

	// Initializing Parameters
	std::vector<unsigned int> cpu_schedule;
	unsigned int sched_iteration = 0;
    std::vector<double> performances;
    std::vector<int> actions;
    std::vector<bool> performances_update_inds;
    for (unsigned int t=0;t<thread_count;t++){
    	performances.push_back(0);
    	performances_update_inds.push_back(false);
    	actions.push_back(0);
    }

    /*
     * Initializing ThreadControl that incorporates the recording of performance counters.
     */
    ThreadControl thread_control;

    // Running main control loop
	while (active_threads){

		nanosleep(&ts, NULL);
		sched_iteration++;
		std::cout << " sched iteration " << sched_iteration << std::endl;

		active_threads = false;
		std::cout << " The current thread runs on CPU: " << sched_getcpu() << std::endl;

		/**
		 * Performance Counting and Scheduling Update
		 * */
		for (unsigned int t=0;t<thread_count;t++){
			std::cout << " Retrieving measurements for thread " << tinfo[t].thread_id << std::endl;
			if (!thread_control.thd_record_counters(tinfo[t].thread_id,&tinfo[t]))
					printf("Error: Problem recording counters for thread %d", (int)tinfo[t].thread_id);
			performances[t] = tinfo[t].performance;
			performances_update_inds[t] = tinfo[t].performance_update_ind;
			actions[t] = tinfo[t].cpu_schedule;
			tinfo[t].performance_update_ind = false;
			if (tinfo[t].status == 0){
				active_threads_vec[t] = true;	// the thread has not complete its task.
				active_threads = true;
			}
			else
				active_threads_vec[t] = false;
		}

		if (active_threads){
			update( performances, actions, sched_iteration, performances_update_inds, active_threads_vec, RL_mapping );
			cpu_schedule = get_cpu_schedule();
		}

		/**
		 * Applying Scheduling Policy
		 * **/
		for (unsigned int i = 0; i < thread_count; i++)
		{
			if ( tinfo[i].status == 0 )
			{
				/*
				 * Suspending thread before setting its affinity
				 */
				if (suspend_threads)
				{
					printf ("Suspending thread %d.\n", i);
					if (thd_suspend (tinfo[i].thread_id) != 0){
						printf ("%s:%d\t ERROR: Suspend thread failed!\n", __FILE__, __LINE__);
					}
				}

				/*
				 * Setting its CPU affinity
				 */
				cpu_set_t mask;
				CPU_ZERO (&mask);
				if (RL_mapping){
					CPU_SET( cpu_schedule[i] , &mask);
					std::cout << " thread " << tinfo[i].thread_num << " will run on CPU " << cpu_schedule[i] << std::endl;
					//CPU_SET( 0 , &mask);
					/*if (tinfo[i].thread_num == 1 || tinfo[i].thread_num == 2)
						CPU_SET( 0 , &mask);
					else
						CPU_SET( 1 , &mask);*/
				}
				else if (OS_mapping){
					for (unsigned int cpu = 0; cpu < num_cpus; cpu++){
						CPU_SET(cpu,&mask);
					}
				}

				/*
				 * bind process to processor
				 * */
				if (pthread_setaffinity_np(tinfo[i].thread_id, sizeof(mask), &mask) <0)
				{
					printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
				}

				/*
				 * Continuing running thread after setting its affinity
				 */
				if (suspend_threads){
					printf ("Continuing thread %d.\n", i);
					if (thd_continue (tinfo[i].thread_id) != 0){
						printf ("%s:%d\t ERROR: Continuing thread failed!\n", __FILE__, __LINE__);
					}
				}

			}
			else
				std::cout << " Status of thread " << i << ": FINISHED!" << " ( time = " << tinfo[i].termination_time << " )" << std::endl;
		}

	}
}
