#include <assert.h>
#include <cstring>
#include <cerrno>

#include "TaskQueue.hxx"

#ifdef SCHEDULER
#include "ThreadInfo.h"
#include "ThreadControl.h"
#include "Scheduler.h"
#include <unistd.h>
#include <sys/syscall.h>

extern Scheduler scheduler;
extern pthread_t sched_thread;
#endif

/// This class extends MultiThreadedSyncPrimitive by including m_suspended member.
class MultiThreadedScheduler: public MultiThreadedSyncPrimitive
{
public:
  
  MultiThreadedScheduler() : m_suspended(false), MultiThreadedSyncPrimitive() {
  }
  
  ~MultiThreadedScheduler() {
  }
  
  inline int suspended() const {
    return m_suspended;
  }
  
  // This function hides MultiThreadedSyncPrimitive::suspend().
  inline void suspend(bool locked = false, int value = true) {
    int code;
    if (locked == false) code = lock();
    m_suspended = value;
    code = MultiThreadedSyncPrimitive::suspend();
    code = unlock();
  }
  
  inline void resume() {
    m_suspended = false;
    MultiThreadedSyncPrimitive::resume();
  }
  
private:
  
  int m_suspended;
};

#ifdef SCHEDULER
void *run_async_wrapper(void *in)
{
  scheduler.run();
  return NULL;
}

// Run scheduler in asynchronous mode (in a separate OS thread)
pthread_t run_scheduler_async()
{
  pthread_attr_t attr;
  pthread_t sched_thread;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  pthread_create(&sched_thread, &attr, run_async_wrapper, NULL);
  return sched_thread;
}
#endif


/// This class is neither visible nor accessible by the application.
class MultiThreadedTaskQueueServer: public MultiThreadedTaskQueue, public MultiThreadedSyncPrimitive
{
  
protected:
  
  MultiThreadedTaskQueueServer(int)
    : MultiThreadedTaskQueue(),
      MultiThreadedSyncPrimitive(),
      m_thread(NULL) 
  {
    //PRINT(this);
    m_finished_jobs = 0;
    memset(m_waitingClients, 0, sizeof(m_waitingClients));
  }
  
  ~MultiThreadedTaskQueueServer() {
    clean(false);
  }
  
  inline void clean(bool force_exit_from_thread_function = true) {
    if (m_threads <= 0) {
      return;
    }
    
    if (force_exit_from_thread_function) {
      lock();
      // Let all active threads know we're finished.
      int nt = m_threads;
      m_threads = -1;
      // Gently push suspended threads allowing them to discover
      // that they should return from threadFunc.
      for (int i = 0; i < nt; i++) {
        m_scheduler[i].resume();
      }
      unlock();
      
      // Wait till all threads do just that.
      int i = 0;
      while (i < nt) {
        for (; i < nt; i++) {
          if (m_scheduler[i].suspended() != -1) {
            //_mm_pause();
            sched_yield();
            break;
          }
        }
      }
    }
    
    if (m_thread)
      delete [] m_thread;
    if (m_client)
      delete [] m_client;
    if (m_scheduler)
      delete [] m_scheduler;
  }
  
public:
  
  friend class MultiThreadedTaskQueue;
  
  /// Without arguments, suspend the server,
  /// With integer argument, suspend i-th task.
  using MultiThreadedSyncPrimitive::suspend;
  inline static void suspend(int i) {
    assert(m_client[i] == NULL);
    m_scheduler[i].suspend();
  }
  
  /// Without arguments, resume the server.
  /// With integer argument, resume i-th task.
  using MultiThreadedSyncPrimitive::resume;
  using MultiThreadedSyncPrimitive::resumeAll;
  inline static void resume(int i) {
    assert(m_client[i]);
    m_scheduler[i].resume();
  }
  
  void createThreads(int threads) {
#ifdef SCHEDULER
    thread_info *tinfo = scheduler.get_tinfo();
#endif
    assert(threads >= 1);
    
    // Free resources.
    clean();
    
    typedef MultiThreadedTaskQueue* pMultiThreadedTaskQueue;
    m_threads   = threads;
    m_thread    = new pthread_t[threads];
    m_client    = new pMultiThreadedTaskQueue[threads];
    m_scheduler = new MultiThreadedScheduler[threads];
    memset(m_client, 0, sizeof(pMultiThreadedTaskQueue) * threads);
    
    //#ifdef SCHEDULER
    //extern pthread_t sched_thread;
    //#endif
    int res;

    for (int i = 0; i < m_threads; i++) {
#ifdef SCHEDULER
      tinfo[i].thread_num = i;
      if ( (res = pthread_create(&tinfo[i].thread_id, NULL, &threadFunc, (void*)i)) != 0) {
	
#else
      if ( (res = pthread_create(&m_thread[i], NULL, &threadFunc, (void*)i)) != 0) {
#endif
	cerr << "can't create thread " << i;
	if (res == EAGAIN)
	  cerr << " (the system does not have enough resources)" << endl;
	else
          cerr << ". reason : " << res << endl;
        exit(EXIT_FAILURE);
      }

#ifdef SCHEDULER
      m_thread[i] = tinfo[i].thread_id;
#endif
    }

#ifdef SCHEDULER
    sched_thread = run_scheduler_async();
#endif
    // Wait till all threads suspend themselves for the first time.
    waitUntillAllThreadsAreSuspended();
  }

  inline bool finished() const { return m_threads == -1; }

private:

  // All other member functions are not accessible by derived classes.

  inline void waitUntillAllThreadsAreSuspended() {
    int ns;
    do {
      //_mm_pause(); // sleep(0);
      sched_yield();
      ns = 0;
      for (int i = 0; i < m_threads; i++)
        ns += m_scheduler[i].suspended();
    } while (ns != m_threads);
  }


  static void* threadFunc(void* _id) {
    long long int id = (long long int)_id;

#ifdef SCHEDULER
    ThreadControl thread_control;
    thread_info *tinfo = scheduler.get_tinfo();
    if(!thread_control.thd_init_counters (tinfo[(int)id].thread_id, (void *)tinfo))
    printf("Error: Problem initializing counters for thread %d",tinfo[(int)id].thread_num);

    /*
     * Get the pid of the process
     */
    tinfo[(int)id].tid = syscall(SYS_gettid);
#endif


    // First time...
    m_scheduler[id].suspend();

    int seqn;
    long long int tag = -1;
    while (!m_server.finished()) {
      MultiThreadedTaskQueue* client = MultiThreadedTaskQueue::m_client[id];

      int action = 0;

      assert(client);
      if (tag != client->tag()) {
        tag = client->tag();
        m_server.lock();
        if (client->m_assigned_jobs >= client->m_threads) {
          m_server.unlock();
          goto getnext;
        }
        seqn = client->m_assigned_jobs++;
        m_server.unlock();
      }
      action = client->task(seqn, (int)id);
      if (action == THREAD_EXIT)
        break;

    getnext:
      // deactivateThread also suspends it if there are no more jobs pending.
      seqn = deactivateThread((int)id);
    }

    m_scheduler[id].suspend(false, -1);

    //#ifdef SCHEDULER
    //tinfo[(int)id].status = 1;
    //tinfo[(int)id].termination_time = tinfo[(int)id].time_before - tinfo[(int)id].time_init;
    //#endif

    return NULL;
  }


  inline static int deactivateThread(const int threadID) {

    // Clear up threadID job.
    MultiThreadedTaskQueue* client = m_client[threadID];
    assert(m_client[threadID]);
    m_client[threadID] = NULL;

    // Check if this client is finished and
    // schedule a new job for the current thread.

    int code = m_server.lock();
    int seqn = -1;
    //printf("%i", (1 + client->m_finished_jobs));

    // If done, server will be waked up (it was suspended in waitForAllThreads).
    bool done = ++client->m_finished_jobs == client->m_threads;

    // Is there any new job for this thread?
    if (m_server.schedule(threadID)) {
      // Yep, unlock and return to the threadFunc
      // (after resuming the server if we done).
      if (done)
        m_server.resume();
      else
        seqn = client->m_assigned_jobs++;
      code = m_server.unlock();
    } else {
      // Nope, wait here till addClient resumes this thread.
      // (after resuming the server if we done).
      if (done)
        m_server.resume();
      m_scheduler[threadID].lock();
      code = m_server.unlock();
      m_scheduler[threadID].suspend(true);
    }
    return seqn;
  }


  inline int getNextClientIndex() {
    // This function doesn't lock anything; caller should do it.
    int j = m_finished_jobs;
    for (int i = 0; i < queuesize; i++) {
      if (m_waitingClients[j]) {
        return j;
      }
      if (++j == queuesize) j = 0;
    }
    return -1;
  }


  void addClient(MultiThreadedTaskQueue* client) {

  recheck:
    m_server.lock();
    int j = m_finished_jobs;
    for (int i = 0; i < queuesize; i++) {
      if (m_waitingClients[j] == NULL) goto found;
      if (++j == queuesize) j = 0;
    }
    // Should be extremely rare occasion; worth signaling fatal error...
    // But just for shear fun of it...
    m_server.unlock();
    //_mm_pause(); // sleep(0);
    sched_yield();
    goto recheck;

  found:

    m_waitingClients[j] = client;
    m_scheduledJobs[j] = 0;

    m_finished_jobs = j + 1;
    if (m_finished_jobs == queuesize) m_finished_jobs = 0;

    int nt = m_server.m_threads;
    // For all free threads among nt server threads...
    for (int i = 0; i < nt; i++) {
      m_scheduler[i].lock();
      if (m_scheduler[i].suspended()) {
        if (!schedule(i)) {
          m_scheduler[i].unlock();
          break;
        }
        m_scheduler[i].resume();
      }
      m_scheduler[i].unlock();
    }

    m_server.unlock();
  }


  bool schedule(int i) {
    // This function doesn't lock anything; caller should do it.
    if (m_client[i] == NULL) {

      int ji = getNextClientIndex();
      if (ji == -1) {
        // There are no more pending jobs.
        return false;
      }
      m_client[i] = m_waitingClients[ji];

      // Are we done assigning threads for this client?
      if (++m_scheduledJobs[ji] == m_waitingClients[ji]->m_threads)
        m_waitingClients[ji] = NULL; // yep, remove it from the queue

    }
    return true;
  }

  pthread_t*              m_thread;                    // active server threads
  static const int        queuesize = 4;               // queue size
  MultiThreadedTaskQueue* m_waitingClients[queuesize]; // client queue
  int                     m_scheduledJobs[queuesize];  // # of already scheduled jobs per client
  };


/// This is a public function.
void MultiThreadedTaskQueue::setMaxNumberOfThreads(int threads) {
  //PRINT(threads);
  m_server.createThreads(threads);
}
/// This is a public function.
  int MultiThreadedTaskQueue::maxNumberOfThreads() {
    return m_server.numberOfThreads();
}

/// This is a public function.
void MultiThreadedTaskQueue::createThreads(int threads) {
  assert(threads >= 1);
  m_threads = threads;
  m_server.lock();
  if (m_server.m_threads == 0) {
    // Initialize server (once).
    m_server.createThreads(threads);
  }
  m_server.unlock();
}


/// This is a public function.
void MultiThreadedTaskQueue::startThreads() {
  m_finished_jobs = m_assigned_jobs = 0;
  m_server.addClient(this);
}

/// This is a public function.
void MultiThreadedTaskQueue::waitForAllThreads() {
  if (m_finished_jobs == m_threads) {
    return;
  }
  int code = m_server.lock();
  //printf("%c", char('a' + m_finished_jobs));

  // Server is suspended if threads are stil running...
  while (m_finished_jobs < m_threads) {
    m_server.suspend();
  }
  //printf("\n");
  
  code = m_server.unlock();
}

/// Initialize static members.
MultiThreadedTaskQueueServer MultiThreadedTaskQueue::m_server(1);
MultiThreadedTaskQueue** MultiThreadedTaskQueue::m_client = NULL;
MultiThreadedScheduler*  MultiThreadedTaskQueue::m_scheduler = NULL;
  

