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
 * ThreadControl.h
 *
 *  Created on: Feb 10, 2016
 *      Author: Georgios Chasparis
 *      Description: This library collects functions with respect to the control of the operation of a thread, the collection of performance counters, etc.
 */

#ifndef THREADCONTROL_H_
#define THREADCONTROL_H_

#include <iostream>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>
#include <papi.h>

#include "ThreadInfo.h"


class ThreadControl
{
public:
	ThreadControl();
	~ThreadControl();

	bool thd_init_counters (pthread_t target_thread, void* arg);
	bool thd_record_counters (pthread_t thread, void* arg);


private:

};


#endif  /* THREADCONTROL_H_ */

