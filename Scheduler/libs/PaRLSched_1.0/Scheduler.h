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

#include "ThreadInfo.h"

class Scheduler
{
public:
	Scheduler();
	~Scheduler();
	Scheduler(const unsigned int& num_cpus, const unsigned int& num_threads, const double& step_size, const double& LAMBDA,
			const bool & printout_strategies, const bool & printout_actions, const bool& write_to_files,
			const bool& write_to_files_details, const double& gamma);

	void update(std::vector<double>& performances, std::vector<int>& actions, int iteration, std::vector< bool >& update_inds,
			std::vector< bool >& active_threads_vec, const bool& RL_mapping);

	inline const std::vector<unsigned int> & get_cpu_schedule(void) const
	{
		return (cpu_schedule_);
	}

	void run(const unsigned int & num_threads, const unsigned int& num_cpus, const bool & OS_mapping, const bool & RL_mapping,
			const double & sched_period, const bool & suspend_threads, thread_info* tinfo);



private:

	unsigned int num_cpus_;
	unsigned int num_threads_;
	double step_size_;
	double LAMBDA_;
	double gamma_;

	bool printout_strategies_;
	bool printout_actions_;
	bool write_to_files_;
	bool write_to_files_details_;

	std::vector< double > performances_;
	std::vector< double > balanced_performances_;
	double cur_average_performance_;	// current average performance
	double cur_balanced_performance_;	// current balanced performance
	double run_average_performance_;	// running average performance
	double run_average_balanced_performance_;	// running average balanced performance
	std::vector< double > run_average_performances_;
	std::vector< std::vector < double > > run_average_performances_cpu_;
	std::vector< std::vector < double > > run_average_balanced_performances_cpu_;
	std::vector< double > min_performances_;
	std::vector< double > alg_performances_;

	std::vector<unsigned int> cpu_schedule_;
	std::vector< std::vector<double> > com_map_strategies_;
	std::vector< std::vector<double> > map_strategies_;

	double time_;
	double time_before_;

	unsigned int num_active_threads_;
	unsigned int num_active_threads_before_;

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
