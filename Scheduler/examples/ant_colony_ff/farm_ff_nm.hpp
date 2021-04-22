
#ifndef FARM_FF_NM_HPP_
#define FARM_FF_NM_HPP_

#include <ff/farm.hpp>
#include <ff/node.hpp>
#include <ff/cycle.h>
#include <ff/spin-lock.hpp>
#include <ff/mapper.hpp>
#include <ff/mapping_utils.hpp>

#include <numa.h>
#include <sched.h>
#include <getopt.h>
#include <time.h>

#include "ant_engine.hpp"

using namespace ff;

// ----- variables for topology description -------------------------------------
static char **topo = NULL;             // table containing the cores in each NUMA node
static int *wrkTable = NULL;           // contains the NUMA node of each worker
static int *wrkCore = NULL;            // contains the core where each worker is pinned
int nnodes;
int ncpus;
// ------------------------------------------------------------------------------

struct ffTask_t {
  ffTask_t(int id, int tn, int nd) :
    ant_id(id), taskn(tn), node(nd) { }

  int ant_id, taskn, node;
};

// discover underlying topology and sets cpu list for FF mapper
void buildTopology() {
	if (numa_available() < 0) {
		std::cerr << "No NUMA API available" << std::endl;
		exit(EXIT_FAILURE);
	}

	//char myList1[] = "2,8,16,24,3,9,17,25,4,10,18,26,5,11,19,27,6,12,20,28,7,13,21,29,0,14,22,30,1,15,23,31";
	ncpus  = numa_num_task_cpus();
	nnodes = numa_max_node()+1;

	int len = 2*ncpus*sizeof(int)+1;
	char *nList = (char *) ::malloc(len);
	memset(nList,'\0',1);

	/* static list of NUMA nodes, containing cores in each node
	 * E.G.:
	 *	topo[0] = 0,1,2,3,4,5,6,7
	 *	topo[1] = 8,9,10,11,12,13,14,15
	 *	etc.
	 */
	topo = (char **) ::malloc(nnodes*sizeof(char*));

	struct bitmask *cpus = numa_allocate_cpumask();
	char *pus = (char*) ::malloc(ncpus*sizeof(char));
	for(int j = 0; j < nnodes; ++j) {
		topo[j] = (char *) ::malloc(2*cpus->size*sizeof(char));
		memset(topo[j],'\0',1);
		memset(pus,'\0',1);
		if (numa_node_to_cpus(j, cpus) >= 0) {
			for (unsigned int i = 1; i < cpus->size; i++)
				if (numa_bitmask_isbitset(cpus, i) && i < 32) { // exclude double context
					sprintf(pus, " %d", i);
					strcat(nList, pus);
					strcat(topo[j], pus);
				}
		}
		strcat(topo[j], " ");
	}
	::free(pus);
	numa_free_nodemask(cpus);

	//printf("Nlist: %s\n", nList);

	// set CPUs list for FF load balancer
	threadMapper::instance()->setMappingList(nList);
	::free(nList);
	//threadMapper::instance()->setMappingList(myList1);
}

class Ant_Worker: public ff_node {
public:
	Ant_Worker() { }

	int svc_init() {
		srand(time(NULL));
		return 1;
	}

	void svc_end() { }

	void *svc(void * task) {
		ffTask_t *t = (ffTask_t *) task;
		cost[t->ant_id] = solve(t->ant_id, weight[t->node], deadline[t->node], process_time[t->node], tau[t->node]);

		return GO_ON;
	}
};


/*
 * Custom load balancer to force our scheduling policy
 */
class my_loadbalancer: public ff_loadbalancer {
protected:
	// virtual function in ff_loadbalancer
	inline size_t selectworker() { return victim; }
public:
	my_loadbalancer(int max_num_workers) : ff_loadbalancer(max_num_workers) {}
	inline void set_victim(int v) { victim=v; }
private:
	int victim;
};


class Ant_Emitter: public ff_node {
public:
	Ant_Emitter(int len, int wrks, svector<ff_node*> workers, my_loadbalancer *lb) :
		ntask(len), nnd(-1), wrks(wrks), wrkrs_(workers), lb(lb) {}

	int svc_init() {
		char id[8];
		for (int j=0; j<wrks; ++j) {
			wrkCore[j] = threadMapper::instance()->getCoreId(lb->getTid(wrkrs_[j]));
			sprintf(id, " %d ", wrkCore[j]);
			for (int k=0; k<nnodes; ++k)
				if(strstr(topo[k], id) != NULL)
					wrkTable[j] = k;
		}
		return 0;
	}

	void svc_end() {

		//printf("END\n");
	}

	void *svc(void *ts) {

		for(unsigned i=0; i<num_ants; ++i) {
			// choose worker
			next = i%wrks;
			lb->set_victim(next);
			nnd = wrkTable[next];

			ff_send_out(new ffTask_t(i, ntask, nnd));
		}
		return (void*) FF_EOS; // done
	}

private:
	int ntask;
	int next;
	int nnd;
	int wrks;
	svector<ff_node *> wrkrs_;
	my_loadbalancer *lb;
};





#endif /* FARM_FF_V2_HPP_ */
