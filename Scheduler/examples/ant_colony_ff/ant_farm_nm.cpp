#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <fstream>

#ifdef USE_NUMA_H
#include "farm_ff_nm.hpp"
#else
#include "farm_ff_v1.hpp"
#endif

#include "ant_engine.hpp"

using namespace ff;

void init(const char *fname, unsigned int num_ants) {
	unsigned int num_tasks;
	std::ifstream infile(fname);

	if (infile.is_open()) {
		infile >> num_jobs >> num_tasks;

#ifdef USE_NUMA_H

		tau[0] = (double*) ::malloc(sizeof(double)*num_jobs*num_jobs);
	    weight[0] = (unsigned int *) ::malloc(sizeof(unsigned int)*num_jobs);
	    deadline[0] = (unsigned int *) ::malloc(sizeof(unsigned int)*num_jobs);
	    process_time[0] = (unsigned int *) ::malloc(sizeof(unsigned int)*num_jobs);

		for(int j=0; j<num_jobs; j++)
			infile >> process_time[0][j];
		for(int j=0; j<num_jobs; j++)
			infile >> weight[0][j];
		for(int j=0; j<num_jobs; j++)
			infile >> deadline[0][j];

		for (int j=0; j<num_jobs; j++)
			for (int i=0; i<num_jobs; i++)
				tau[0][i*num_jobs+j] = 1.0;
#else

		tau = (double*) ::malloc(sizeof(double)*num_jobs*num_jobs);
	    weight = (unsigned int *) ::malloc(sizeof(unsigned int)*num_jobs);
	    deadline = (unsigned int *) ::malloc(sizeof(unsigned int)*num_jobs);
	    process_time = (unsigned int *) ::malloc(sizeof(unsigned int)*num_jobs);

	    for(int j=0; j<num_jobs; j++)
	      infile >> process_time[j];
	    for(int j=0; j<num_jobs; j++)
	      infile >> weight[j];
	    for(int j=0; j<num_jobs; j++)
	      infile >> deadline[j];

	    for (int j=0; j<num_jobs; j++)
	    	for (int i=0; i<num_jobs; i++)
	    		tau[i*num_jobs+j] = 1.0;
#endif

		all_results = (unsigned int **) ::malloc(sizeof(unsigned int*)*num_ants);
		for (int j=0; j<num_ants; j++)
			all_results[j] = (unsigned int *) ::malloc(sizeof(unsigned int)*num_jobs);

		best_result = (unsigned int *) ::malloc(sizeof(unsigned int)*num_jobs);
	} else {
		std::cerr << "[ERROR] Input file does not exist" << std::endl;
		exit(EXIT_FAILURE);
	}
	infile.close();
}

void clean() {

#ifdef USE_NUMA_H

	for(unsigned i=1; i<nnodes; ++i) {
		numa_free(tau[i], sizeof(double)*num_jobs*num_jobs);
		numa_free(weight[i], sizeof(unsigned int)*num_jobs);
		numa_free(deadline[i], sizeof(unsigned int)*num_jobs);
		numa_free(process_time[i], sizeof(unsigned int)*num_jobs);
	}
	::free(tau[0]);
	::free(weight[0]);
	::free(deadline[0]);
	::free(process_time[0]);

	::free(tau);
	::free(weight);
	::free(deadline);
	::free(process_time);	

	if(wrkCore)
	  free(wrkCore);
	if(wrkTable)
	  free(wrkTable);
	if(topo)
	  for(int j=0; j<nnodes; ++j)
	    if(topo[j])
	      ::free(topo[j]);
#else
	::free(tau);
	::free(weight);
	::free(deadline);
	::free(process_time);
#endif
	::free(cost);
	::free(best_result);

	if(all_results)
	  //  for(int j=0; j<num_ants; ++j)
	  //::free(all_results[j]);
	  ::free(all_results);
}


int main(int argc, char **argv) {

	unsigned int num_iter=100;
	std::string fname ("inputs/wt2000.txt");
	unsigned int i, j;
	unsigned int num_workers, nw, chunk_size=1;
	nw = ff_realNumCores();

	if (argc<5) {
		std::cerr << " Usage: " << argv[0] << " <nr_workers> <nr_iterations> <nr_ants> <input_filename>" << std::endl;
		num_workers = nw;
		std::cerr << " Using default parameters:\n "
				<< argv[0] << " " << num_workers << " " << num_iter << " "
				<< num_ants << " \'" << fname << "\' " << "\n" << std::endl;
	} else {
		num_workers = atoi(argv[1]);
		num_iter = atoi(argv[2]);
		num_ants = atoi(argv[3]);
		fname.assign(argv[4]);
		//do_chunking = atoi(argv[5]);

		if(num_workers > nw) num_workers = nw;
	}

#ifdef USE_NUMA_H
	buildTopology();
	wrkTable = (int *) ::malloc(num_workers*sizeof(int));
	wrkCore  = (int *) ::malloc(num_workers*sizeof(int));

	if(nnodes>0) {
		tau = (double**) ::malloc(sizeof(double*)*nnodes);
		weight = (unsigned int **) ::malloc(sizeof(unsigned int*)*nnodes);
		deadline = (unsigned int **) ::malloc(sizeof(unsigned int*)*nnodes);
		process_time = (unsigned int **) ::malloc(sizeof(unsigned int*)*nnodes);
	} else {
		std::cerr << "Can't detect NUMA nodes. Aborting..." << std::endl;
		exit(EXIT_FAILURE);
	}
#endif

	init(fname.c_str(), num_ants);
	cost = (unsigned int *) ::malloc(sizeof(unsigned int)*num_ants);

#ifdef USE_NUMA_H

	for (int i=1; i<nnodes; i++) {
		weight[i] = (unsigned int *) numa_alloc_onnode(sizeof(unsigned int)*num_jobs, i);
		memcpy(weight[i], weight[0], sizeof(unsigned int)*num_jobs);
		deadline[i] = (unsigned int *) numa_alloc_onnode(sizeof(unsigned int)*num_jobs, i);
		memcpy(deadline[i], deadline[0], sizeof(unsigned int)*num_jobs);
		process_time[i] = (unsigned int *) numa_alloc_onnode(sizeof(unsigned int)*num_jobs, i);
		memcpy(process_time[i], process_time[0], sizeof(unsigned int)*num_jobs);
		tau[i] = (double *) numa_alloc_onnode(sizeof(double)*num_jobs*num_jobs, i);
		memcpy(tau[i], tau[0], sizeof(double)*num_jobs*num_jobs);
	}
#endif

	// create the farm object
	ff_farm<my_loadbalancer> farm;
	std::vector<ff_node*> w;
	for(int i=0;i<num_workers;++i)
		w.push_back(new Ant_Worker());
	farm.add_workers(w);

	Ant_Emitter e(num_iter, num_workers, farm.getWorkers(), farm.getlb());
	e.setAffinity(0);
	farm.add_emitter(&e);

	t1 = elapsedTime(ST);

	for(j=0; j<num_iter; ++j) {
		if(farm.run_and_wait_end() < 0) {
			std::cerr << "Farm error!" << std::endl;
			exit(EXIT_FAILURE);
		}
		best_t = pick_best(&best_result);
#ifdef USE_NUMA_H
		update(best_t, best_result, nnodes);
#else
		update(best_t, best_result);
#endif
	}

	t1 = elapsedTime(SP);

//	ff::ParallelFor pfr(num_workers, true /* nonblocking */);
//	pfr.disableScheduler();
//	pfr.parallel_for(0,num_ants,1, [] (const long i) {i;}); // warm up
//
//	for (j=0; j<num_iter; j++) {
//		#pragma omp parallel for shared(cost,chunk_size) private(i) schedule(static, chunk_size)
//		pfr.parallel_for_static(0,num_ants,1, num_ants/num_workers, [&] (const long i){
//		pfr.parallel_for(0,num_ants,1, [&] (const long i){
//			cost[i] = solve(i);
//		});
//		best_t = pick_best(&best_result);
//		update(best_t, best_result);
//	}

	std::cout << "Best solution found has cost " << best_t << std::endl;
	std::cerr << "Running time: " << t1 << " ms." << std::endl;

	clean();
}


