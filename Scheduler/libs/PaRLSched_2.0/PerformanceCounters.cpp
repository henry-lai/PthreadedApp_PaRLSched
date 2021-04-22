/* ./src/mlpp/optim/StopCriterion.hpp
 * -----------------------------------------------------------------------------
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
 * PerformanceCounters.cpp
 *
 *  Created on: Feb 11, 2016
 *      Author: chasparis
 */

#include "PerformanceCounters.h"
#include <fstream>
#include <vector>
#include <iostream>

PerformanceCounters::PerformanceCounters(void)
{
	values = new long long int ( {0} );
	EventSet = PAPI_NULL;
}

PerformanceCounters::~PerformanceCounters(void)
{
	delete values;
}

int PerformanceCounters::initialize(void)
{

	int retval;

	/* Initialize the PAPI library */
	retval = PAPI_library_init(PAPI_VER_CURRENT);
	if (retval != PAPI_VER_CURRENT) {
		fprintf(stderr, "PAPI library initialization error!\n");
		return(1);
	}

	/* Create the Event Set */
	if (PAPI_create_eventset(&EventSet) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Instructions Executed to our EventSet */
	if (PAPI_add_event(EventSet, PAPI_TOT_INS) != PAPI_OK)	// PAPI_TOT_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Cycles to our EventSet */
	if (PAPI_add_event(EventSet, PAPI_TOT_CYC) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Add Total Instructions Executed to our EventSet */
	if (PAPI_add_event(EventSet, PAPI_LST_INS) != PAPI_OK) //PAPI_LST_INS
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	/* Start counting */
	if (PAPI_start(EventSet) != PAPI_OK)
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);

	return(0);

}

void PerformanceCounters::get_performance(Performance& performance)
{

	/* Read Performances */
	if (PAPI_read(EventSet, values) != PAPI_OK){
		printf ("%s:%d\t ERROR\n", __FILE__, __LINE__);
	}

	std::cout << " test " << performance.initial_tot_ins << ", " << values[0] << " ( difference: " << (double)values[0] - performance.initial_tot_ins << " )" << std::endl;

	performance.initial_tot_ins = (double)values[0] - performance.initial_tot_ins;
	performance.initial_tot_cyc = (double)values[1] - performance.initial_tot_cyc;
	performance.initial_lst_ins = (double)values[2] - performance.initial_lst_ins;

	gettimeofday(&tim, NULL);
	performance.time = (double)tim.tv_sec+((double)tim.tv_usec/1000000.0) - performance.time;

	std::cout << " tot ins + " << performance.initial_lst_ins << std::endl;
	std::cout << " tot cyc + " << performance.initial_tot_cyc << std::endl;
	std::cout << " lst_ins + " << performance.initial_tot_ins << std::endl;
	std::cout << " time      " << performance.time << std::endl;

}
