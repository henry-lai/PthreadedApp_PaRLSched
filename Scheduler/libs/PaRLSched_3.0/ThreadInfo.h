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
 * ThreadInfo.h
 *
 *  Created on: Apr 29, 2016
 *      Author: Georgios Chasparis
 *      Description: This library collects variables related to the creation & operation of a thread.
 */

#ifndef SRC_THREADINFO_H_
#define SRC_THREADINFO_H_

#include "MethodsActions.h"

struct thread_info
{    /* Used as argument to thread_start() */
	thread_info(){};
   pthread_t 			thread_id;         				/* ID returned by pthread_create() */
   unsigned long int	papi_thread_id;					/* PAPI thread ID */
   int       			thread_num;        				/* Application-defined thread # */
   char     			*argv_string;       			/* From command-line argument */
   int long 			total_instructions_completed;	/* Total instructions completed */
   //std::vector<Struct_Actions>		s_actions_;						/* Structure Defining Actions of the Threads  */
   bool					status;							/* Status boolean */
   double				termination_time;
   unsigned int			thread_goal;
   unsigned int			thread_ref;						/* references related to the objective */
   double				performance;
   double 				performance_before;				/* previous performance */
   bool					performance_update_ind;			/* indicator that the performance has been updated */
   double				time_init;
   double				time_before;					/* this is the time of the last performance measurement */
   double 				time;
   int					EVENT_SET;						/* This is the event set of the performance counters */
   pid_t				tid;
   pid_t				pid;
   pthread_t		return_thread_id(void)
   {
	   return thread_id;
   }
   unsigned int 		memory_index;					// an index that defines which part of the memory is used

};


#endif /* SRC_THREADINFO_H_ */
