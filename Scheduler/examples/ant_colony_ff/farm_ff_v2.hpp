/*
 * TODO: FINISH THIS
 */
#ifndef FARM_FF_V2_HPP_
#define FARM_FF_V2_HPP_

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
		if(!ts) {
			for(unsigned i=0; i<num_ants; ++i) {
				// choose worker
				next = i%wrks;
				lb->set_victim(next);
				ff_send_out(new ffTask_t(i, i, nnd));
			}
			return GO_ON;
		} else {
			ntask--;
			for(unsigned i=0; i<num_ants; ++i) {
				// choose worker
				next = i%wrks;
				lb->set_victim(next);
				ff_send_out(new ffTask_t(i, i, nnd));
			}
			if((ntask-1) == 0) return EOS;
			else return GO_ON;
		}
		return (void*) EOS; // never reached
	}

private:
	int ntask;
	int next;
	int nnd;
	int wrks;
	svector<ff_node *> wrkrs_;
	my_loadbalancer *lb;
	int streamlen;
};

class Ant_Collector : public ff_node {
public:
	Ant_Collector() { }

	void *svc(void *ts) {
		if(ts) {
			++received;
			if(received == (num_ants-1)) {
				best_t = pick_best(&best_result);
				return ts;
			}
		}

		return NULL;
	}
private:
	long received;
};

#endif /* FARM_FF_V2_HPP_ */
