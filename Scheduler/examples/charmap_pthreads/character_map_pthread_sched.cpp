#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <map>
#include <string>

#include <pthread.h>
#include <time.h>


#ifdef SCHEDULER
#include "SchedulerParams.h"
thread_info *tinfo;
#endif

#define MAX_THREADS 128

#define NUM_RUNS 100000000 //100mil  to make times at least > 100 secs

pthread_t threadsTable[MAX_THREADS];
int threadsTableAllocated[MAX_THREADS];
pthread_mutexattr_t normalMutexAttr;
int nThreads;
int numThreads = MAX_THREADS;


/*
Takes a string and returns a  character map
*/
std::map<char,int>GetCharMap(std::string InputString) 
{
    char key; int charCount = 0;
    std::map<char, int> charMap; //Instantiate map

    for (size_t i = 0; i < InputString.size(); i++) {
        charCount = std::count (InputString.begin(),InputString.end(),InputString.at(i));  //Count occurences of current char
        key = InputString.at(i); //insert that char as the key for that pair
        charMap.insert( std::pair<char, int>(key,charCount));//insert key and value into charMap
    }
    //for (std::map<char,int>:: iterator it =charMap.begin(); it!=charMap.end(); ++it)
      // printf("%c:%d ",it-> first, it->second);
    //printf("\n");
    return charMap;
}

/*
Runs the GetCharMap and repeats the function N num of times to make program do work
*/
void *RunCharMap(void *tid_ptr)
{
    std::string str1 ("This is a line");
    std::string InputString = str1;
    int tid = *(int *)tid_ptr;

#ifdef SCHEDULER
	ThreadControl thread_control;
	thread_info * info = &(tinfo[tid]);
	if(!thread_control.thd_init_counters (info->thread_id, (void *)info))
	  printf("Error in init counters for thread %d", info->thread_num);
	info->tid = syscall(SYS_gettid);
#endif    

    for(int i = 0; i < NUM_RUNS; i++)
        GetCharMap(InputString);
    pthread_exit(NULL);

#ifdef SCHEDULER
	info->status = 1;
	info->termination_time = info->time_before - info->time_init;
#endif

 return (void *)NULL;
}


int main(int argc, char *argv[])
{
    /*
	Starting timer
   */
    clock_t start, end;
    start = clock();

        int i;      
	if(argc !=3){
	 printf("Usage:\n\t%s <nthread> <outputfile>\n", argv[0]);
	 exit(1);
	}
    nThreads = atoi(argv[1]);
    char *outputFile = argv[2];

    #ifdef SCHEDULER
      Scheduler scheduler(nThreads);
    #endif


    pthread_mutexattr_init( &normalMutexAttr ); 
    for( i = 0; i < MAX_THREADS; i++) {
	threadsTableAllocated[i] = 0; 
    }
    //printf("\nnumber of Runs: %d\n", NUM_RUNS);
    printf("\nnumber of threads: %d \n", nThreads);

    int *tids;
    tids = (int *) malloc (nThreads * sizeof(int));

    #ifdef SCHEDULER
        tinfo = scheduler.get_tinfo(); //access thread info in ThreadInfo.h file
    #endif
    for ( i=0; i < nThreads; i++) {
        tids[i] = i;
    
    #ifdef SCHEDULER
        //printf("creating thread %d \n", i);
        tinfo[i].thread_num = i;
        tinfo[i].status = 0;
    #endif

    #ifdef SCHEDULER
        //printf("initializng pthreads\n");   
        pthread_create(&tinfo[i].thread_id,NULL,(void *(*)(void *))RunCharMap, (void *) &tids[i]);
        threadsTable[i] = tinfo[i].thread_id;
    #else
        pthread_create(&threadsTable[i],NULL,(void *(*)(void *))RunCharMap, (void *) &tids[i]);
    #endif
    threadsTableAllocated[i] = 1;
    }
   
    #ifdef SCHEDULER
        scheduler.run();
    #else
    
    void *ret;
    for(i =0; i<MAX_THREADS; i++) {
	if( threadsTableAllocated[i] == 0) break; 
        pthread_join(threadsTable[i], &ret);
    }
    #endif

    //calculate program execution time
    end = clock();
    double time_taken = double(end - start) / double(CLOCKS_PER_SEC);
    std::cout << time_taken << std::endl;
    printf ("\n");

    //write execution time to file
    std::ofstream myfile;
    myfile.open(outputFile, std::fstream::in | std::fstream::app);
    if(myfile.is_open()) {
      myfile << "time taken: " << time_taken << "\n";
      myfile.close();
    }
    else std:: cout << "Unable to open file\n";

    free(tids);
    pthread_exit(NULL);
    return 0;
}

//compile with g++ -pthread character_map_pthread.cpp -o character_map_pthread
