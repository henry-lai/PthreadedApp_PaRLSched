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
 *  Created on: Nov 29, 2016
 *      Author: Georgios C. Chasparis
 * Description: Provides methods and information structuring necessary for creating
 * 				estimates over performances of allocations.
 */

#ifndef METHODSESTIMATE_H_
#define METHODSESTIMATE_H_

#include <vector>
#include <iostream>
#include <set>



/*
 * Struct_Estimate
 *
 * @description: provides the necessary information for keeping track our estimates for the performance of a thread over a specific node/cpu
 * or the performance of a thread's memory over a specific node (or some further refinement if possible)
 *
 * This structure is planned for 'per-thread' use, since it will hold estimates over the proper use of resources by each thread separately.
 *
 * Within this structure, there is an additional struct that tracks the performances of the threads throughout the running time.
 *
 */
struct Struct_Estimate
{

	std::string resource_;									/* The name of the resource assigned. */
	unsigned int num_sources_;								/* This is the number of the sources available, e.g., number of CPU's available. */
	std::vector< std::vector< unsigned int > > vec_child_sources_;		/* This is the number of the child sources available for each one of the main sources.
																Potentially, there might be different number of child sources for each main source. */
	std::vector<double> vec_estimates_;						/* This vector holds our estimates over the best assignment, e.g., over the CPU assignment */
	std::vector<double> vec_cummulative_estimates_;
	double run_average_performance_;						/* It holds our running average performance */
	std::vector< Struct_Estimate > vec_child_estimates_;	/* child estimates (used under hierarchical resources) */

	/*
	 * This function initializes the structure
	 */
	void initialize(const std::string& resource, const unsigned int& num_sources, const unsigned int& max_num_sources_user)
	{
		resource_ = resource;
		num_sources_ = std::min<unsigned int>(num_sources,max_num_sources_user);
		vec_estimates_.resize(num_sources_);
		vec_cummulative_estimates_.resize(num_sources_);
		if (num_sources > 0)
		{
			for (unsigned int e=0;e<num_sources_;e++)
			{
				vec_estimates_.at(e) = (double)1/((double)num_sources);
				if (e==0)
					vec_cummulative_estimates_[e] = vec_estimates_[e];
				else
					vec_cummulative_estimates_[e] = vec_cummulative_estimates_[e-1] + vec_estimates_[e];
			}
		}
	}


	/*
	 * initialize_w_child
	 * @description: It is implicitly assumed that the main resource also has a child resource
	 * For example, in the case of PROCESSING_BANDWIDTH, the main sources correspond to the NUMA nodes (if available)
	 * while the child sources correspond to the CPU units available in each NUMA node.
	 */
	void initialize_w_child
		(
			  const std::string& resource
			, const unsigned int& num_sources
			, const unsigned int& max_num_sources_user
			, const std::string& child_resource
			, const std::vector< std::vector< unsigned int > >& vec_num_child_sources
			, const std::vector< unsigned int >& vec_max_num_child_sources_user
		)
	{
		resource_ = resource;
		num_sources_ = std::min<unsigned int>(num_sources,max_num_sources_user);
		vec_estimates_.resize(num_sources_);
		vec_cummulative_estimates_.resize(num_sources_);
		vec_child_sources_.clear();
		for (unsigned int r=0;r < num_sources_; r++){
			std::vector<unsigned int> temp_child_resources;
			unsigned int num_child_res = std::min<unsigned int>(vec_num_child_sources[r].size(),vec_max_num_child_sources_user[r]);
			for (unsigned int cr=0; cr < num_child_res; cr++)
				temp_child_resources.push_back(vec_num_child_sources[r][cr]);
			vec_child_sources_.push_back(temp_child_resources);
		}
		/* initialize the vector of estimates for the main resource */
		if (num_sources > 0){
			for (unsigned int e=0;e<num_sources_;e++)
			{
				vec_estimates_.at(e) = (double)1/((double)num_sources_);
				if (e==0)
					vec_cummulative_estimates_[e] = vec_estimates_[e];
				else
					vec_cummulative_estimates_[e] = vec_cummulative_estimates_[e-1] + vec_estimates_[e];
			}
		}
		/* initialize the vector of estimates for the child resources */
		vec_child_estimates_.clear();
		if (vec_child_sources_.size() > 0){
			// in case the number of the child resources is nonzero
			for (unsigned int cr = 0; cr < vec_child_sources_.size(); cr++)
			{
				Struct_Estimate s_estimate;
				s_estimate.initialize(child_resource, vec_child_sources_[cr].size(), vec_max_num_child_sources_user[cr]);
				vec_child_estimates_.push_back(s_estimate);
			}
		}
	}

	/*
	 * This function receives a vector of estimates, and assigns it to the existing structure
	 */
	void update_estimates(const std::vector<double>& vec_estimates)
	{
		for (unsigned int b=0;b<vec_estimates.size();b++)
			vec_estimates_.at(b) = vec_estimates.at(b);
	}

	void update_child_estimate(const std::vector<double>& vec_estimates, const unsigned int& source_num)
	{
		for (unsigned int b=0;b<vec_estimates.size();b++)
			vec_child_estimates_.at(source_num).vec_estimates_.at(b) = vec_estimates.at(b);
	}

	/*
	 * Update Actions
	 */
	void update_action(const std::vector<double>& vec_estimates, const unsigned int & source_num){};

};


/*
 * MethodsEstimate:
 * Description: This structure provides alternative methodologies for creating estimates over the best selection.
 * Inputs: For now, we assume that it takes as inputs the following:
 * 			- prior estimate data (maybe not created under the same rule)
 * 			- current performance (this may take different values depending on the resource to be allocated)
 */
struct Struct_MethodsEstimate
{
	/*
	 * RL_update
	 * @description: Provides a framework for creating estimates over the best action, by using the current selection and the current performance
	 * It is assumed that a set of 'finite' number of actions/selections is available (e.g., number of available nodes).
	 */
	void RL_update(std::vector<double>& vec_estimates, std::vector<double>& vec_cummulative_estimates, const double& current_performance, const unsigned int& current_action, const double& step_size)
	{
		for (unsigned int source=0;source<vec_estimates.size();source++)
		{
			/* for each one of the sources, we need to update the corresponding vector of estimates */
			if (source == current_action)
			{
				/* in case, we update the source corresponding to the current_action */
				vec_estimates[source] = vec_estimates[source] + step_size * (double)current_performance * ( 1 - vec_estimates[source] );
			}
			else
			{
				vec_estimates[source] = vec_estimates[source] - step_size * (double)current_performance * vec_estimates[source];
			}
		}

		// computing the new cumulative estimates
		for (unsigned int source=0;source<vec_estimates.size();source++)
		{
			vec_cummulative_estimates[source] = (double)1/((double)vec_estimates.size());
			if (source==0)
				vec_cummulative_estimates[source] = vec_estimates[source];
			else
				vec_cummulative_estimates[source] = vec_cummulative_estimates[source-1] + vec_estimates[source];
		}
	}

};


#endif /* METHODSESTIMATE_H_ */
