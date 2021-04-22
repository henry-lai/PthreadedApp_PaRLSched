
#ifndef FF_FARM_V1_HPP_
#define FF_FARM_V1_HPP_

#include <ff/farm.hpp>
#include <ff/node.hpp>
#include <ff/cycle.h>
#include <ff/spin-lock.hpp>
#include <ff/mapper.hpp>
#include <ff/mapping_utils.hpp>

#include <time.h>

#include "ant_engine.hpp"

using namespace ff;

struct ffTask_t {
	ffTask_t(int id, int tn, int nd) :
		ant_id(id), taskn(tn), node(nd) { }

	int ant_id, taskn, node;
};

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
		cost[t->ant_id] = solve(t->ant_id, weight, deadline, process_time, tau);

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
		return 0;
	}

	void svc_end() {
	}

	void *svc(void *ts) {

		for(unsigned i=0; i<num_ants; ++i) {
			// choose worker
			next = i%wrks;
			lb->set_victim(next);
			ff_send_out(new ffTask_t(i, i, nnd));
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



#endif /* FF_FARM_V1_HPP_ */
