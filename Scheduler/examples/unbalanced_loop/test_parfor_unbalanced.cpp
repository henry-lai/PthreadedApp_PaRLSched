
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

#include <ff/utils.hpp>
#include <ff/parallel_for.hpp>

#if defined(USE_TBB)
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/task_scheduler_init.h>
#endif


using namespace ff;

#define MY_RAND_MAX         32767
#define WORKING_SET_BYTES (64 * (1<<20))
#define CACHELINESIZE	64

__thread unsigned long next = 1;
static long *arr2 = NULL;
static long *arr3 = NULL;

unsigned long getCacheLineSize() {
  unsigned long mem=0;
  FILE *fp;
  char cmd[256];

  // read 'coherency_line_size' file for cache of index '0'
  cmd[0]='\0';
  strcpy(cmd,"cat /sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size");

  fp = popen(cmd,"r");
  if(fscanf(fp,"%ld",&mem)>0)
    fclose(fp);

  return mem;
}


inline static long Random(void) {
  next = random() * 1103515245 + 12345;
  return (((long) (next/65536) % (MY_RAND_MAX+1)) / (MY_RAND_MAX +1.0)) + 1;
}

inline void funcToCompute(long *arr) {
  long c=0, e=0, res=0;
  c = rand() % ((WORKING_SET_BYTES/4)-2);
  e = rand() % (WORKING_SET_BYTES-1);

  if(c>e) c-=e;

  // perform some r/m/w operations on random 
  // portions of arrays
  for(volatile long j=c;j<e;++j) {
    res = arr[j]+1;
    res += arr2[j]+res;
    res += arr3[j]+arr[j];
  }
}

int main(int argc, char *argv[]) {
  long seqIter=44, inIter=99;
  int nthreads = 4;
  long *g_ptr = nullptr;
  
  if (argc < 4) {
    printf(" USAGE: %s out_iters in_iters num_workers\n", argv[0]);
    printf(" -> using default params: %s %ld %ld %d.\n", argv[0], seqIter,inIter,nthreads);
  } else {
    seqIter  = atol(argv[1]);
    inIter   = atol(argv[2]);
    nthreads = atoi(argv[3]);
  }

  printf(" --> Working set: %ld MB\n", (WORKING_SET_BYTES*sizeof(long)) >> 20);
  printf(" Using %d threads; %ld outer steps; %ld inner steps\n", nthreads, seqIter, inIter);

  // this will contain execution times for each worker thread
  std::vector<std::vector<double> > wrktimes(nthreads);
  for(int f=0; f<nthreads; ++f)
    wrktimes[f].reserve(seqIter*inIter);

  srandom(time(NULL));

  // ------ PLACEMENT NEW CAN BE USED WITH LIBNUMA AND STL CONTAINERS ------
  //	std::vector<long> *g_ptr;
  //	void *p = ::malloc(sizeof(long)+24); // numa_alloc_onnode / numa_alloc_interleave
  //	g_ptr = new (p) std::vector<long>();
  //	g_ptr->reserve(WORKING_SET_BYTES);
  // -----------------------------------------------------------------------

#ifdef USE_NUMA
  //  ***** NUMA *****
  //
  if(numa_available() < 0) {
    std::cerr << "not a numa machine!" << std::endl;
    return -1;
  }
  
  g_ptr = (long*) numa_alloc_interleaved(WORKING_SET_BYTES*sizeof(long));
  
  // **** OR ****

  int ncpus  = numa_num_task_cpus();
  int nnodes = numa_max_node() + 1;
  
  //for(auto n : nnodes)
  //  g_ptr = (long*) numa_alloc_onnode(WORKING_SET_BYTES*sizeof(long), n);
  
  // static list of NUMA nodes, with cores in each node
  char **topo = (char **) ::malloc(nnodes*sizeof(char*));
  struct bitmask *cpus = numa_allocate_cpumask();
  char *pus = (char*) ::malloc(ncpus*sizeof(char));
  
  for(int j = 0; j < nnodes; ++j) {
    topo[j] = (char *) ::malloc(2*cpus->size*sizeof(char));
    memset(topo[j],'\0',1);
    memset(pus,'\0',1);
    if (numa_node_to_cpus(j, cpus) >= 0) {
      for (unsigned int i = 1; i < cpus->size; i++)
	if (numa_bitmask_isbitset(cpus, i) && i < ff_realNumCores()) { // exclude double context
	  sprintf(pus, " %d", i);
	  
	  strcat(nList, pus);
	  strcat(topo[j], pus);
	}
    }
    strcat(topo[j], " ");
  }
  ::free(pus);
  numa_free_nodemask(cpus);
  
  //  FastFlow will handle thread pinning
  // -----------------------------------------------------

#else

  // allocate three big arrays (largely exceed L3 cache size)
  g_ptr = (long*) ::malloc(WORKING_SET_BYTES*sizeof(long));
  arr2  = (long*) ::malloc(WORKING_SET_BYTES*sizeof(long));
  arr3  = (long*) ::malloc(WORKING_SET_BYTES*sizeof(long));

#endif

  //std::vector<long> g_ptr(WORKING_SET_BYTES,0);
  //long *g_ptr = new long[WORKING_SET_BYTES]();

  for(long i=0; i<WORKING_SET_BYTES; ++i) {
    g_ptr[i] = (Random() % MY_RAND_MAX)/10 ;
    arr2[i]  = (Random() % MY_RAND_MAX)/10 ;
    arr3[i]  = (Random() % MY_RAND_MAX)/10 ;
  }

  printf(" Arrays filled!\n");

  ParallelFor ffpf( nthreads, (nthreads < ff_numCores()) );
  ffpf.disableScheduler(); // disable scheduler

  for(int k=0; k<seqIter; ++k) {

    ffpf.parallel_for_thid(0,inIter,1,0,[g_ptr,&wrktimes](const unsigned i, const int thid) {

	ffTime(START_TIME);
	
	funcToCompute(g_ptr);
	
	ffTime(STOP_TIME);
	wrktimes[thid].push_back(ffTime(GET_TIME));

      }, nthreads);

  } // outer for loop

  for(int t=0; t<nthreads; ++t) {
    double sum = 0.0;
    double mean = 0.0;
    sum = std::accumulate(wrktimes[t].begin(), wrktimes[t].end(), 0.0);
    mean = sum / wrktimes[t].size();
    printf(" - Worker %d - mean execution time: %lf ms.\n", t, mean);
  }

#ifdef LOG

  printf("\n ** Workers execution time (ms):\n");
  for(int t=0;t<nthreads; ++t) {
    printf(" - Worker %d:", t);
    for(unsigned f=0; f<wrktimes[t].size(); ++f) {
      printf(" %lf", wrktimes[t][f]);
    }
    printf("\n");
  }
  printf("\n");

#endif


#ifdef USE_NUMA

  // numa_free(g_ptr, WORKING_SET_BYTES*sizeof(long));
  // numa_free(arr2, WORKING_SET_BYTES*sizeof(long));
  // numa_free(arr3, WORKING_SET_BYTES*sizeof(long));

#else

  ::free(g_ptr); ::free(arr2); ::free(arr3);
  //delete[] g_ptr;
  //::free(p);  

#endif


  return 0;
}
