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
 * MethodsOptimize.h
 *
 *  Created on: Dec 12, 2016
 *      Author: Georgios C. Chasparis
 * Description: Provides methods and information structuring necessary for optimizing the
 * 				resources for each one of the threads.
 */

#ifndef METHODSOPTIMIZE_H_
#define METHODSOPTIMIZE_H_

#include <vector>
#include <iostream>
#include <set>



/*
 * Struct_Optimize
 *
 * @description: It provides the necessary functions/algorithms for optimizing the selection of resources from the threads.
 * 				It may or may not make use of the estimates over the most rewarding resources, as created by the MethodsEstimate.h library.
 *
 */
struct Struct_Optimize
{

};


struct Struct_MethodsOptimize
{
	/*
	 * RL_optimize
	 *
	 * @description: Provides a framework for computing the optimal allocation of resources based on the estimates created by an RL estimator.
	 *
	 * @inputs: The function receives as inputs
	 * 			a) prior estimates
	 * 			b) prior actions
	 * 			and computes the next allocation that needs to be implemented by all threads.
	 */
	void RL_optimize(Struct_PerformanceMonitoring& Performance, Struct_Estimate& Estimate, Struct_Actions& Action, const double& LAMBDA)
	{
		/*
		 * Updating Main Resource
		 */
		double rnd = rand() % 100;
		unsigned int num_choices = Action.num_actions_main_resource_;

		// selecting main source
		if (rnd <= (LAMBDA / Performance.balanced_performance_) * 100)
		{
			// this is the case where we perturb the action (with a probability that it is rather small)
			// we then select one of the available choices by using a uniform distribution
			Action.action_per_main_source_ = random_selection_uniform(num_choices);
		}
		else
			Action.action_per_main_source_ = random_selection_strategy(num_choices, Estimate.vec_cummulative_estimates_);

		/*
		 * Updating Selections for Child Resources
		 * We do update the choice of a child resource only for the main resource selected previously
		 */
		rnd = rand() % 100;
		unsigned int cur_main_source = Action.action_per_main_source_;
		unsigned int num_child_choices = Action.vec_num_child_actions_per_main_resource_[cur_main_source];
		if (rnd <= (LAMBDA / Performance.balanced_performance_) * 100)
		{
			// this is the case where we perturb the action (with a probability that it is rather small)
			// we then select one of the available choices by using a uniform distribution
			Action.vec_actions_per_child_source_[cur_main_source] = random_selection_uniform(num_child_choices);
		}
		else
			Action.vec_actions_per_child_source_[cur_main_source] = random_selection_strategy(num_child_choices, Estimate.vec_child_estimates_[cur_main_source].vec_cummulative_estimates_);

	};


	unsigned int random_selection_uniform(const unsigned int& num_choices)
	{
		unsigned int action(0);
		double rnd = rand() % 100;
		for (unsigned int a = 0; a < num_choices; a++)
		{
			// for each one of the main sources, we perform the following steps

			if ( (a==0) && (rnd >=0) && ( rnd < (double)(a+1) * (1/(double)num_choices) * 100 )  )
			{
				action = a;
				break;
			}
			else if ( (a>0) && (rnd >= (double)a * (1/(double)num_choices) * 100) && (rnd < (double)(a+1) * (1/(double)num_choices) * 100))
			{
				action = a;
				break;
			}
		}

		return action;
	};


	unsigned int random_selection_strategy(const unsigned int& num_choices, const std::vector<double>& estimates)
	{
		// here, we follow a policy based on the current strategy
		unsigned int action(0);
		double rnd = rand() % 100;
		for (unsigned int a = 0; a < num_choices; a++)
		{
			if ((a==0) && (rnd >=0) && (rnd < estimates[a] * 100)){
				action = a;
				break;
			}
			else if ((a > 0)&&(rnd >= estimates[a-1] * 100) && (rnd < estimates[a] * 100)){
				action = a;
				break;
			}
		}

		return action;
	}

};


#endif /* METHODSOPTIMIZE_H_ */
