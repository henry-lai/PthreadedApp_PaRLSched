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
 * ThreadControl.cpp
 *
 *  Created on: May 2, 2016
 *      Author: Georgios Chasparis
 */

#include "ThreadControl.h"

//#include <boost/bind.hpp>
//#include <boost/function.hpp>


ThreadControl::ThreadControl()
{
};

ThreadControl::~ThreadControl()
{
};

pthread_mutex_t mut_init_counters = PTHREAD_MUTEX_INITIALIZER;

thread_info * object;

pthread_t wrapper(void)
{
	object->return_thread_id();
}

/*
 * The function thd_initialize_counters intends on initializing the PAPI counters
 * Inputs:
 * 			pthread_t: an integer ID of this thread
 * 			arg: the info related to this thread
 */
bool ThreadControl::thd_init_counters (pthread_t target_thread, void* arg)
{

//	std::cout << "Initializing Performance Counters " << std::endl;

	thread_info * info = (static_cast<thread_info*>(arg));

	/*
	 * In this part, I was experimenting with replacing the pthread_self() command, that retrieves the ID of the thread
	 * but it may only run within the thread itself.
	 * Now the ID is retrieved through the info of each thread, and by assigning the corresponding ID to the global variable 'wrapper'
	 * This initialization works properly, which means that this function can be called outside the thread_execute function (i.e., it can be
	 * called from the scheduler). But, I haven't tried that yet.
	 */
	/*pthread_mutex_lock(&mut_init_counters);

	object = new thread_info(*info);

	// boost::function<pthread_t()> pointer_fct = boost::bind(&thread_info::return_thread_id, info);

	// Testing
	pthread_t temp = pthread_self();

	std::cout << " the outcome of the pthread_self() of thread " << object->thread_num << " is " << temp << std::endl;
	std::cout << " the outcome of the pointer to the function is " << object->return_thread_id() << std::endl;


	if (PAPI_thread_init(&wrapper) != PAPI_OK)			// in the place of
		 printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	pthread_mutex_unlock(&mut_init_counters); */

	if (PAPI_thread_init(pthread_self) != PAPI_OK)
		 printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	info->EVENT_SET = PAPI_NULL;

	/* Create the Event Set */
	if (PAPI_create_eventset(&info->EVENT_SET) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Instructions Executed to our EventSet */
	if (PAPI_add_event(info->EVENT_SET, PAPI_TOT_INS) != PAPI_OK)	// PAPI_TOT_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Cycles to our EventSet */
	if (PAPI_add_event(info->EVENT_SET, PAPI_TOT_CYC) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Instructions Executed to our EventSet */
	if (PAPI_add_event(info->EVENT_SET, PAPI_LST_INS) != PAPI_OK) //PAPI_LST_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Cycles Stalled Waiting for memory Reads
	if (PAPI_add_event(info->EVENT_SET, PAPI_MEM_SCY) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);*/

	/* Start counting */
	if (PAPI_start(info->EVENT_SET) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	info->performance = 0;
	info->performance_before = 0;

	/*
	 * Initializing time
	 */
	struct timeval tim;
	gettimeofday(&tim, NULL);

	info->time_init = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	info->time_before = info->time_init;

	return true;
}

/*bool ThreadControl::thd_init_counters (pthread_t target_thread, thread_info* info)
{

	std::cout << "Initializing Performance Counters " << std::endl;

	// thread_info * info = (static_cast<thread_info*>(arg));


	 * In this part, I was experimenting with replacing the pthread_self() command, that retrieves the ID of the thread
	 * but it may only run within the thread itself.
	 * Now the ID is retrieved through the info of each thread, and by assigning the corresponding ID to the global variable 'wrapper'
	 * This initialization works properly, which means that this function can be called outside the thread_execute function (i.e., it can be
	 * called from the scheduler). But, I haven't tried that yet.

	pthread_mutex_lock(&mut_init_counters);

	object = new thread_info(*info);

	// boost::function<pthread_t()> pointer_fct = boost::bind(&thread_info::return_thread_id, info);

	// Testing
	pthread_t temp = pthread_self();

	std::cout << " the outcome of the pthread_self() of thread " << object->thread_num << " is " << temp << std::endl;
	std::cout << " the outcome of the pointer to the function is " << object->return_thread_id() << std::endl;


	if (PAPI_thread_init(&wrapper) != PAPI_OK)			// in the place of
		 printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	pthread_mutex_unlock(&mut_init_counters);

	if (PAPI_thread_init(pthread_self) != PAPI_OK)
		 printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	info->EVENT_SET = PAPI_NULL;

	 Create the Event Set
	if (PAPI_create_eventset(&info->EVENT_SET) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	 Add Total Instructions Executed to our EventSet
	if (PAPI_add_event(info->EVENT_SET, PAPI_TOT_INS) != PAPI_OK)	// PAPI_TOT_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	 Add Total Cycles to our EventSet
	if (PAPI_add_event(info->EVENT_SET, PAPI_TOT_CYC) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	 Add Total Instructions Executed to our EventSet
	if (PAPI_add_event(info->EVENT_SET, PAPI_LST_INS) != PAPI_OK) //PAPI_LST_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	 Cycles Stalled Waiting for memory Reads
	if (PAPI_add_event(info->EVENT_SET, PAPI_MEM_SCY) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	 Start counting
	if (PAPI_start(info->EVENT_SET) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);



	info->performance = 0;
	info->performance_before = 0;


	 * Initializing time

	struct timeval tim;
	// double time0(0);
	gettimeofday(&tim, NULL);

	info->time_init = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	info->time_before = info->time_init;

	// std::cout << " The current time is : " << info->time_init << std::endl;

	return true;
}*/

bool ThreadControl::thd_init_counters (pthread_t thread_id, thread_info& info)
{


	/*
	 * In this part, I was experimenting with replacing the pthread_self() command, that retrieves the ID of the thread
	 * but it may only run within the thread itself.
	 * Now the ID is retrieved through the info of each thread, and by assigning the corresponding ID to the global variable 'wrapper'
	 * This initialization works properly, which means that this function can be called outside the thread_execute function (i.e., it can be
	 * called from the scheduler). But, I haven't tried that yet.
	 */
	pthread_mutex_lock(&mut_init_counters);
	info.thread_id = thread_id;

//	if (PAPI_thread_init((unsigned long (*) (void)) (thread_num)) != PAPI_OK)
	if (PAPI_thread_init(pthread_self) != PAPI_OK)
		 printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	info.EVENT_SET = PAPI_NULL;

	/* Create the Event Set */
	if (PAPI_create_eventset(&info.EVENT_SET) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Instructions Executed to our EventSet */
	if (PAPI_add_event(info.EVENT_SET, PAPI_TOT_INS) != PAPI_OK)	// PAPI_TOT_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Cycles to our EventSet */
	if (PAPI_add_event(info.EVENT_SET, PAPI_TOT_CYC) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Instructions Executed to our EventSet */
	if (PAPI_add_event(info.EVENT_SET, PAPI_LST_INS) != PAPI_OK) //PAPI_LST_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Cycles Stalled Waiting for memory Reads
	if (PAPI_add_event(info->EVENT_SET, PAPI_MEM_SCY) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);*/

	/* Start counting */
	if (PAPI_start(info.EVENT_SET) != PAPI_OK){
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
	}
	/*else
		std::cout << " PAPI started for thread_id = " << info.thread_id << std::endl;*/

	info.performance = 0;
	info.performance_before = 0;

	/*
	 * Initializing time
	 */
	struct timeval tim;
	gettimeofday(&tim, NULL);

	info.time_init = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	info.time_before = info.time_init;

	pthread_mutex_unlock(&mut_init_counters);

	return true;
}

bool ThreadControl::thd_stop_counters (const int & thread_id, thread_info& info)
{

	std::cout << "Terminating Performance Counters " << std::endl;

	/*
	 * In this part, I was experimenting with replacing the pthread_self() command, that retrieves the ID of the thread
	 * but it may only run within the thread itself.
	 * Now the ID is retrieved through the info of each thread, and by assigning the corresponding ID to the global variable 'wrapper'
	 * This initialization works properly, which means that this function can be called outside the thread_execute function (i.e., it can be
	 * called from the scheduler). But, I haven't tried that yet.
	 */
	pthread_mutex_lock(&mut_init_counters);

// 	object = new thread_info(*info);
	long long int values[3];
	if (PAPI_stop(info.EVENT_SET, values) != PAPI_OK){
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
		exit(1);
	}

	pthread_mutex_unlock(&mut_init_counters);

	return true;
}


/*
 * thd_record_counters
 * Records the selected counters for this thread
 * Inputs:
 * 			- pthread_t thread: the ID # of this thread
 * 			- void* arg:		the info of this thread
 */
bool ThreadControl::thd_record_counters (pthread_t thread, void* arg)
{

	// std::cout << "Retrieving Performance Counters" << std::endl;
	thread_info * info = (static_cast<thread_info*>(arg));

	if (info->status != 0)
		return true;

	/* Read Performances */
	long long int values[3];
	if (PAPI_read(info->EVENT_SET, values) != PAPI_OK){
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
	}

	/*
	 * Retrieving the current time
	 */
	struct timeval tim;
	double current_time;
	gettimeofday(&tim, NULL);
	current_time = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	info->time = current_time;

	/*
	 * Updating the performance of this thread...
	 */

	info->performance = ((double)values[0] - info->performance_before)/(info->time - info->time_before);
	info->performance_before = (double)values[0];
	info->performance_update_ind = true;

	// std::cout << " Thread : " << info->thread_num << " has current performance " << info->performance << " on time interval " << info->time - info->time_before << std::endl;

	/*
	 * Updating the elapsed time
	 */
	info->termination_time += info->time - info->time_before;

	/*
	 * Updating the last recording time
	 */
	info->time_before = current_time;



	return true;
}

bool ThreadControl::thd_record_counters (thread_info& info)
{

	// std::cout << "Retrieving Performance Counters" << std::endl;
	// thread_info * info = (static_cast<thread_info*>(arg));

	if (info.status != 0)
		return true;

	/* Read Performances */
	long long int values[3];
	if (PAPI_read(info.EVENT_SET, values) != PAPI_OK){
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
	}

	/*
	 * Retrieving the current time
	 */
	struct timeval tim;
	double current_time;
	gettimeofday(&tim, NULL);
	current_time = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0);
	info.time = current_time;

	/*
	 * Updating the performance of this thread...
	 */

	info.performance = ((double)values[0] - info.performance_before)/(info.time - info.time_before);
	info.performance_before = (double)values[0];
	info.performance_update_ind = true;

	// std::cout << " Thread : " << info->thread_num << " has current performance " << info->performance << " on time interval " << info->time - info->time_before << std::endl;

	/*
	 * Updating the elapsed time
	 */
	info.termination_time += info.time - info.time_before;

	/*
	 * Updating the last recording time
	 */
	info.time_before = current_time;



	return true;
}

