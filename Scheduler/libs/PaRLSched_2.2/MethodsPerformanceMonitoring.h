/*
 * MethodsPerformanceMonitoring.h
 *
 *  Created on: Dec 07, 2016
 *      Author: Georgios C. Chasparis
 * Description: Provides methods and information structuring necessary for performance aggregation.
 */

#ifndef METHODSPERFORMANCEMONITORING_H_
#define METHODSPERFORMANCEMONITORING_H_

#include <vector>
#include <iostream>
#include <set>


/*
 * Struct_Overall_PerformanceMonitoring
 *
 * @description: The role of this structure is some overall information regarding the performance of the threads
 * e.g., the sum of the performances of the threads
 */
struct Struct_OverallPerformance
{
	std::vector< double >	sum_performances_per_main_resource_;			/* This is the sum of the performances over the main resource choice */
	std::vector< double >   sum_balanced_performances_per_main_resource_;
	std::vector< double >   ave_performance_per_main_resource_;
	std::vector< double >   ave_balanced_performance_per_main_resource_;

	std::vector< double >   run_average_performance_;
	std::vector< double >   run_average_balanced_performance_;

	void initialize(const unsigned int& num_main_resources)
	{
		sum_performances_per_main_resource_.clear();
		ave_performance_per_main_resource_.clear();
		sum_balanced_performances_per_main_resource_.clear();
		ave_balanced_performance_per_main_resource_.clear();
		run_average_performance_.clear();
		run_average_balanced_performance_.clear();
		for (unsigned int r = 0; r < num_main_resources; r++)
		{
			sum_performances_per_main_resource_.push_back(0);
			ave_performance_per_main_resource_.push_back(0);
			sum_balanced_performances_per_main_resource_.push_back(0);
			ave_balanced_performance_per_main_resource_.push_back(0);
			run_average_performance_.push_back(0);
			run_average_balanced_performance_.push_back(0);
		}
	}

	void update_sum_performances_per_main_resource(const unsigned int& resource_ind, const double& sum_performances)
	{
		sum_performances_per_main_resource_[resource_ind] = sum_performances;
	}
	void update_ave_performance_per_main_resource(const unsigned int& resource_ind, const double& ave_performance)
	{
		ave_performance_per_main_resource_[resource_ind] = ave_performance;
	}
	void update_sum_balanced_performances_per_main_resource(const unsigned int& resource_ind, const double& sum_balanced_performances)
	{
		sum_balanced_performances_per_main_resource_[resource_ind] = sum_balanced_performances;
	}
	void update_ave_balanced_performance_per_main_resource(const unsigned int& resource_ind, const double& ave_balanced_performance)
	{
		ave_balanced_performance_per_main_resource_[resource_ind] = ave_balanced_performance;
	}
	void update_run_average_performance_per_main_resource(const unsigned int& resource_ind, const double& ave_performance, const unsigned int& iteration)
	{
		double step_size = 1/(pow((double)iteration,0.8)+1);
		if (step_size < 0.01)
			step_size = 0.01;
		if (iteration > 10)
		{
			run_average_performance_[resource_ind] = run_average_performance_[resource_ind] + step_size * (ave_performance - run_average_performance_[resource_ind]);
		}
		else
		{
			run_average_performance_[resource_ind] = ave_performance;
		}
	}
	void update_run_average_balanced_performance_per_main_resource(const unsigned int& resource_ind, const double ave_balanced_performance, const unsigned int& iteration)
	{
		double step_size = 1/(pow((double)iteration,0.8)+1);
		if (step_size < 0.01)
			step_size = 0.01;
		if (iteration > 10)
		{
			run_average_balanced_performance_[resource_ind] = run_average_balanced_performance_[resource_ind] +
				step_size * (ave_balanced_performance - run_average_balanced_performance_[resource_ind]);
		}
		else
		{
			run_average_balanced_performance_[resource_ind] = ave_balanced_performance;
		}
	}

};


/*
 * Struct_PerformanceMonitoring
 *
 * @description: An important aspect of creating estimates over future performances is to be able to aggregate prior
 * performances of threads. The possibilities of how to aggregate such performance may vary depending on the application itself.
 *
 * This structure is planned for 'per-thread' use, since it will hold aggregate information over prior performances of a thread.
 *
 */
struct Struct_PerformanceMonitoring
{

	/*
	 * We assume here that there are two kinds of resources (i.e., main resources and child resources)
	 * For example, a NUMA node may refer as the main resource, and the underlying CPU may refer to as a child resource.
	 * For now, it is assumed that there is a 'single' child resource for each main resource, however the current setup
	 * may as well be expanded to the case of multiple child resources.
	 */
	std::string resource_;						// This is the type of resource over which these measurements are recorded.
	unsigned int num_sources_;					// This is the number of choices for the main resource

	/*
	 * Current Performances
	 */
	double performance_;						// Current performance of the thread w.r.t. the resource at hand.
	double balanced_performance_;				// A special type of performance that takes into account the variance from the mean (over all other threads).
	double alg_performance_;					// A special type of performance (to be defined).

	bool performance_update_ind_;				// Indicates whether the performance has been updated

	/*
	 * Overall performance indicators (over all threads)
	 */
	double overall_performance_;
	double overall_balanced_performance_;


	/*
	 * Statistical Properties
	 */
	double min_performance_;					// minimum performance observed so far
	double max_performance_;					// maximum performance observed so far

	/*
	 * Running Average Performances Per Main Resource
	 */
	double run_average_performance_;			// running average performance over the main resource
	double run_average_balanced_performance_;	// running average balanced performance over the main resource

	double run_average_balanced_performance_before_;


	/*
	 * Running Average Performances Per Main & Child Source
	 *
	 * The idea here is that we would like to keep more specialized information about the average performances of each one of the Sources of
	 * either the main Resource or the child resources.
	 *
	 */
	std::vector<unsigned int> vec_num_child_sources_;
	std::vector< double > vec_run_average_performances_per_main_resource_;									// e.g., this is per node running average performance
	std::vector< std::vector < double > > vec_run_average_performances_per_child_resource_;					// e.g., this is per cpu running average performance

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
		num_sources_ = std::min<unsigned int>(num_sources,max_num_sources_user);
		performance_ = 0;
		balanced_performance_ = 0;
		alg_performance_ = 0;
		min_performance_ = 0;
		max_performance_ = 0;
		run_average_performance_ = 0;
		run_average_balanced_performance_ = 0;
		run_average_balanced_performance_before_ = 0;
		for (unsigned int r = 0; r < num_sources_; r++)
		{
			vec_run_average_performances_per_main_resource_.push_back(0);
		}
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
			, const std::vector<unsigned int> vec_max_num_child_sources_user
		)
	{
		performance_ = 0;
		balanced_performance_ = 0;
		alg_performance_ = 0;
		min_performance_ = 0;
		max_performance_ = 0;
		run_average_performance_ = 0;
		run_average_balanced_performance_ = 0;
		run_average_balanced_performance_before_ = 0;
		for (unsigned int r = 0; r < num_sources; r++)
		{
			vec_run_average_performances_per_main_resource_.push_back(0);
			std::vector < double > vec_run_average_performances_per_child_resource;
			unsigned int num_child_sources = std::min<unsigned int>(vec_num_child_sources[r].size(),vec_max_num_child_sources_user[r]);
			for (unsigned int cs=0; cs < num_child_sources; cs++){
				// for each one of the child source, we create an entry for its running average performance
				vec_run_average_performances_per_child_resource.push_back(0);
			}
			vec_run_average_performances_per_child_resource_.push_back(vec_run_average_performances_per_child_resource);
		}

	};

	void update_run_average_performance(const double& performance, const double& step_size, const unsigned int& iteration)
	{
		if (iteration > 10)
			run_average_performance_ = run_average_performance_ + step_size * (performance - run_average_performance_);
		else
			run_average_performance_ = performance;
	}
	void update_run_average_balanced_performance(const double& balanced_performance, const double& step_size, const unsigned int& iteration)
	{
		if (iteration > 10)
			run_average_balanced_performance_ = run_average_balanced_performance_ + step_size * ( balanced_performance - run_average_balanced_performance_);
		else
			run_average_balanced_performance_ = balanced_performance;
	}
	void update_run_average_performance_per_main_resource(const double& performance, const unsigned int& main_source, const double& step_size, const unsigned int& iteration)
	{
		if (iteration > 10)
			vec_run_average_performances_per_main_resource_[main_source] = vec_run_average_performances_per_main_resource_[main_source] +
				step_size * ( performance - vec_run_average_performances_per_main_resource_[main_source]);
		else
			vec_run_average_performances_per_main_resource_[main_source] = performance;
	}
	void update_run_average_performance_per_child_resource(const double& performance, const unsigned int& main_source,
			const unsigned int& child_source, const double& step_size, const unsigned int& iteration)
	{
		if (iteration > 10)
			vec_run_average_performances_per_child_resource_[main_source][child_source] = vec_run_average_performances_per_child_resource_[main_source][child_source] +
				step_size * ( performance - vec_run_average_performances_per_child_resource_[main_source][child_source]);
		else
			vec_run_average_performances_per_child_resource_[main_source][child_source] = performance;
	}


};



#endif /* METHODSPERFORMANCEMONITORING_H_ */
