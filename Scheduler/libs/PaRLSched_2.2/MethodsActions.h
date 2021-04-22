/*
 * MethodsActions.h
 *
 *  Created on: Dec 07, 2016
 *      Author: Georgios C. Chasparis
 * Description: Provides methods and information structuring necessary for the actions of the threads (e.g., cpu, memory placement).
 */

#ifndef METHODSACTIONS_H_
#define METHODSACTIONS_H_

#include <vector>
#include <iostream>
#include <set>


/*
 * Struct_Actions
 *
 * @description: This structure gathers information that is relevant for keeping in track the actions selected by each
 * thread with respect to the resources that need to be optimized (e.g., cpu placement, memory placement)
 *
 */
struct Struct_Actions
{

	/*
	 * We assume here that there are two kinds of resources (i.e., main resources and child resources)
	 * For example, a NUMA node may refer as the main resource, and the underlying CPU may refer to as a child resource.
	 * For now, it is assumed that there is a 'single' child resource for each main resource, however the current setup
	 * may as well be expanded to the case of multiple child resources.
	 */
	std::string resource_;													// This is the type of resource over which these measurements are recorded.
	std::string child_resource_;											// This is the type of child resource
	unsigned int num_actions_main_resource_;								// This is the number of actions per main resource
	std::vector<unsigned int> vec_num_child_actions_per_main_resource_;		// This is the number of actions w.r.t. the child resources per main resource.

	/*
	 * Running Average Performances Per Main & Child Source
	 *
	 * The idea here is that we would like to keep more specialized information about the average performances of each one of the Sources of
	 * either the main Resource or the child resources.
	 *
	 */
	unsigned int action_per_main_source_;				// e.g., this is per node running average performance
	unsigned int previous_action_per_main_source_;
	unsigned int action_per_child_source_;				// e.g., this is per cpu running average performance
	unsigned int previous_action_per_child_source_;
	std::vector < std::vector< unsigned int > > vec_child_sources_;			// e.g., this is the actual indicator of the child source

	/*
	 * initialize()
	 * @description: We simply initialize the performance tracking variables
	 */
	void initialize
		(
				const std::string& resource
			  , const unsigned int& num_sources
			  , const unsigned int& max_num_sources_user
			  , std::vector<unsigned int>& action_main_old
			  , const unsigned int& thread_num
		)
	{
		// We initialize the selections with respect to the main resource (e.g., numa node) to be the first one
		resource_ = resource;
		num_actions_main_resource_ = std::min<unsigned int>(num_sources,max_num_sources_user);
		action_per_main_source_ = 0;
		previous_action_per_main_source_ = 0;
		action_main_old[thread_num] = action_per_main_source_;
	};


	/*
	 * initialize_w_child()
	 * @description: We simply initialize the performance tracking variables when child resources are also present
	 */
	void initialize_w_child
		(
		      const std::string& resource
			, const unsigned int& num_sources
			, const unsigned int& max_num_sources_user
			, const std::string& child_resource
			, const std::vector< std::vector < unsigned int> >& vec_num_child_sources
			, const std::vector< unsigned int >& vec_max_num_child_sources_user
			, const unsigned int& thread_num
			, std::vector<unsigned int>& action_main_old
		)
	{
		/*
		 * We initialize the selections over the main & child sources to be the first choice available
		 */
		resource_ = resource;
		child_resource_ = child_resource;
		num_actions_main_resource_ = std::min<unsigned int>(num_sources, max_num_sources_user);
		vec_num_child_actions_per_main_resource_.clear();
		vec_child_sources_.clear();

		/*
		 * At this point, we have gathered the necessary information regarding the number of actions
		 * regarding the main resources and child resources.
		 */
		std::vector<unsigned int> initial_action = compute_initial_action(vec_num_child_sources, vec_max_num_child_sources_user, thread_num );
		std::cout << " action per main source = " << initial_action[0] << " and child source = " << initial_action[1] << std::endl;
		action_per_main_source_ = initial_action[0];
		std::cout << " action per main source = " << action_per_main_source_ << " and thread number " << thread_num << std::endl;
		action_main_old[thread_num] = action_per_main_source_;
		std::cout << " action main old " << action_main_old[thread_num] << std::endl;
		previous_action_per_main_source_ = initial_action[0];
		action_per_child_source_ = initial_action[1];
		previous_action_per_child_source_ = initial_action[1];

		// Setting the vector of available child resources
		for (unsigned int r = 0; r < num_actions_main_resource_; r++)
		{
			// for each one of the main sources, we assign the first option with respect to the child source.
			unsigned int max_num_child_sources = std::min<unsigned int>(vec_num_child_sources[r].size(), vec_max_num_child_sources_user[r]);
			vec_num_child_actions_per_main_resource_.push_back(max_num_child_sources);
			std::vector<unsigned int> tmp_vec_child_sources;
			for (unsigned int cr = 0; cr < max_num_child_sources; cr ++){
				tmp_vec_child_sources.push_back(vec_num_child_sources[r][cr]);
			}
			vec_child_sources_.push_back(tmp_vec_child_sources);
		}
	};


	/*
	 *
	 * Compute initial action
	 *
	 */

	std::vector<unsigned int> compute_initial_action
		(
				  const std::vector< std::vector < unsigned int> >& vec_num_child_sources
				, const std::vector< unsigned int >& vec_max_num_child_sources_user
				, const unsigned int& thread_num
		)
	{
		// We first compute the correct placement of this thread into the set of CPU's available
		unsigned int overall_counter_cpu = 0;
		unsigned int counter_cpu = 0;
		unsigned int counter_numa = 0;
		bool found_cpu = false;
		while (!found_cpu)
		{
			counter_numa = 0;

			for (unsigned int r = 0; r < vec_num_child_sources.size(); r++)
			{
				// this is a loop over the main resource (e.g., NUMA nodes)
				counter_cpu = 0;
				unsigned int max_num_child_sources_available = std::min<unsigned int>(vec_num_child_sources[r].size(), vec_max_num_child_sources_user[r]);
				unsigned int max_num_child_sources = vec_num_child_sources[r].size();

				for (unsigned int j=0; j < max_num_child_sources_available; j++)
				{
					if ( thread_num == overall_counter_cpu)
					{
						// if the thread number coincides with the cpu number, then
						// we will assign this cpu to the thread
						found_cpu = true;
						break;
					}
					overall_counter_cpu ++;
					counter_cpu++;
				}

				if (found_cpu)
					break;
				else{
					counter_numa ++;
				}

			}
		}

		return {counter_numa, counter_cpu};

	}
};



#endif /* METHODSACTIONS_H_ */
