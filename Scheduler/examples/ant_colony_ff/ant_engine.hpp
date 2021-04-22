
#ifndef ANT_ENGINE_HPP_
#define ANT_ENGINE_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>

#define const_beta 2
#define const_xi 0.1
#define const_rho 0.1
#define const_q0 0.9
#define const_max_int 2147483647
#define const_e 0.000001

unsigned int num_ants=100;
unsigned int num_jobs;

#ifdef USE_NUMA_H
unsigned int **weight;
unsigned int **deadline;
unsigned int **process_time;
double **tau;
#else
unsigned int *weight;
unsigned int *deadline;
unsigned int *process_time;
double *tau;
#endif

unsigned int **all_results;
unsigned int *cost;
unsigned int best_t = UINT_MAX;
unsigned int *best_result;

double t1;

enum { ST, SP, GT };

static inline double elapsedTime(int tag) {
	static struct timeval tv_start = {0,0};
	static struct timeval tv_stop  = {0,0};

	double res=0.0;
	switch(tag) {
	case ST:{
		gettimeofday(&tv_start,NULL);
	} break;
	case SP:{
		gettimeofday(&tv_stop,NULL);
		long sec  = (tv_stop.tv_sec  - tv_start.tv_sec);
		long usec = (tv_stop.tv_usec - tv_start.tv_usec);

		if(usec < 0) {
			usec += 1000000;
		}
		res = ((double)(sec*1000)+ ((double)usec)/1000.0);
	} break;
	case GT: {
		long sec  = (tv_start.tv_sec  - tv_stop.tv_sec);
		long usec = (tv_start.tv_usec - tv_stop.tv_usec);

		if(usec < 0) {
			--sec;
			usec += 1000000;
		}
		res = ((double)(sec*1000)+ ((double)usec)/1000.0);
	} break;
	default:
		res=0;
		break;
	}
	return res;
}

static void mdd( double *res, unsigned int scheduled_time,
		unsigned int *deadline, unsigned int *process_time) {
	unsigned int i;
	for (i=0; i<num_jobs; ++i) {
		if (scheduled_time + process_time[i] > deadline[i])
			res[i] = 1.0 / (scheduled_time + process_time[i] );
		else
			res[i] = 1.0 / (deadline[i] );
	}
}

static void findSolution( unsigned int *results, unsigned int *pt,
		unsigned int *wt, unsigned int *dd, double *pij, double *eta, double *t ) {
	unsigned int i,j,k;
	unsigned int scheduled_time = 0;
	unsigned int remain_time = 0;
	double sumpij = 0;
	unsigned int *tabus = (unsigned int*) ::malloc(sizeof(unsigned int)*num_jobs);
	double q;
	double *tauk;

	double maxp;
	double x;

	memset(tabus, 1, num_jobs * sizeof(unsigned int));

	for (i = 0; i < num_jobs; ++i)
		remain_time += pt[i];

	for (k = 0; k < num_jobs-1; ++k){
		tauk = &t[k*num_jobs];
		mdd(eta, scheduled_time, dd, pt);
		for (i = 0; i < num_jobs; ++i) {
			if (tabus[i] != 0) {
				pij[i] = pow(eta[i], const_beta) * tauk[i];
				sumpij += pij[i];
			} else pij[i] = 0;
		}

		q = ((double)rand())/RAND_MAX;
		if (q < const_q0){
			j = 0;
			maxp = pij[0];
			for (i = 1; i < num_jobs; ++i)
				if (pij[i] > maxp){
					j = i;
					maxp= pij[i];
				}
		} else{
			q = ((double)rand())/RAND_MAX;
			q *= sumpij;
			double temp = 0.0;
			j = 0;
			while(temp - const_e < q && j < num_jobs ){
				temp += pij[j];
				j++;
			}
			j--;
			while ( tabus[j] == 0) j--;
		}

//		if (j>num_jobs)
//			fprintf (stderr, "kuckuricku %d\n", j);

		results[k] = j;
		tabus[j] = 0;
		scheduled_time += pt[j];
		remain_time -= pt[j];
	}

	//	find the last job
	j = 0;
	for (i = 0; i < num_jobs; ++i)
		if (tabus[i] != 0) j = i;

	k = num_jobs-1;
	results[k] = j;
	::free(tabus);
}

unsigned int fitness( unsigned int *result, unsigned int *pt,
	      unsigned int *dd, unsigned int *wt ) {
	unsigned int cost = 0;
	unsigned int i,j;
	unsigned int time = 0;
	for (i = 0; i < num_jobs; ++i){
		j = result[i];
		time += pt[j];
		if (time > dd[j]) {
			cost += (time - dd[j]) * wt[j];
		}
	}
	return(cost);
}

unsigned int solve ( unsigned int ant_id, unsigned int *wt,
	     unsigned int *dd, unsigned int *pt, double *t ) {
	unsigned int cost;
	double *pij = (double*) ::malloc(sizeof(double)*num_jobs);
	double *eta = (double*) ::malloc(sizeof(double)*num_jobs);

	findSolution( all_results[ant_id], pt, wt, dd, pij, eta, t);
	cost = fitness(all_results[ant_id], pt, dd, wt);

	::free(pij);
	::free(eta);

	return(cost);
}

void update( unsigned int best_t, unsigned int *best_result, int nn=0) {
	unsigned int i,j,k;
	double ddd = const_rho / best_t * num_jobs;
	double dmin = 1.0 / 2.0 / num_jobs;

	for (i = 0; i < num_jobs; ++i) {
		for (j = 0; j < num_jobs; ++j) {

#ifdef USE_NUMA_H
			for (k = 0; k < nn; ++k) {
				tau[k][i*num_jobs+j] *= 1-const_rho;
				if (tau[k][i*num_jobs+j] < dmin)
					tau[k][i*num_jobs+j] = dmin;
			}
#else
			tau[i*num_jobs+j] *= 1-const_rho;
			if (tau[i*num_jobs+j] < dmin)
				tau[i*num_jobs+j] = dmin;
#endif
		}
	}

	for (i = 0; i < num_jobs; ++i) {

#ifdef USE_NUMA_H
		for (k = 0; k<nn; k++)
			tau[k][i*num_jobs+best_result[i]] +=  ddd;
#else
		tau[i*num_jobs+best_result[i]] +=  ddd;
#endif
	}
}

unsigned int pick_best(unsigned int **best_result)
{
	unsigned int i,j;
	unsigned int best_t = UINT_MAX;

	for (i=0; i<num_ants; i++) {
		if(cost[i] < best_t) {
			best_t = cost[i];
			*best_result = all_results[i];
		}
	}

	return (best_t);
}




#endif /* ANT_ENGINE_HPP_ */
