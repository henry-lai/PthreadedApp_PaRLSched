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
	unsigned int num_actions_main_resource_;								// This is the number of actions per main resource
	std::vector<unsigned int> vec_num_child_actions_per_main_resource_;		// This is the number of actions w.r.t. the child resources per main resource.

	/*
	 * Running Average Performances Per Main & Child Source
	 *
	 * The idea here is that we would like to keep more specialized information about the average performances of each one of the Sources of
	 * either the main Resource or the child resources.
	 *
	 */
	unsigned int action_per_main_source_;									// e.g., this is per node running average performance
	std::vector < unsigned int > vec_actions_per_child_source_;				// e.g., this is per cpu running average performance
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
		)
	{
		// We initialize the selections with respect to the main resource (e.g., numa node) to be the first one
		resource_ = resource;
		num_actions_main_resource_ = std::min<unsigned int>(num_sources,max_num_sources_user);
		action_per_main_source_ = 0;
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
		)
	{
		/*
		 * We initialize the selections over the main & child sources to be the first choice available
		 */
		resource_ = resource;
		num_actions_main_resource_ = std::min<unsigned int>(num_sources, max_num_sources_user);
		action_per_main_source_ = 0;
		vec_actions_per_child_source_.clear();
		vec_num_child_actions_per_main_resource_.clear();
		vec_child_sources_.clear();

		for (unsigned int r = 0; r < num_actions_main_resource_; r++)
		{
			// for each one of the main sources, we assign the first option with respect to the child source.
			unsigned int max_num_child_sources = std::min<unsigned int>(vec_num_child_sources[r].size(), vec_max_num_child_sources_user[r]);
			vec_actions_per_child_source_.push_back(0);
			vec_num_child_actions_per_main_resource_.push_back(max_num_child_sources);
			std::vector<unsigned int> tmp_vec_child_sources;
			for (unsigned int cr = 0; cr < max_num_child_sources; cr ++){
				tmp_vec_child_sources.push_back(vec_num_child_sources[r][cr]);
			}
			vec_child_sources_.push_back(tmp_vec_child_sources);
		}
	};

};



#endif /* METHODSACTIONS_H_ */
