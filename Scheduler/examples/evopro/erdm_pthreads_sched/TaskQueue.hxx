#ifndef RTTL_THREAD_HXX
#define RTTL_THREAD_HXX

#include "RCBProc.h"

#include <pthread.h>

typedef volatile int atomic_t;

inline int atomic_add(atomic_t *v, const int c) {
  int i = c;
  int __i = i;
  __asm__ __volatile__(
                       "lock ; xaddl %0, %1;"
                       :"=r"(i)
                        :"m"(*v), "0"(i));

  return i + __i;
}

inline int atomic_inc(atomic_t *v) {
  return atomic_add(v,1);
} 

inline int atomic_dec(atomic_t *v) {
  return atomic_add(v,-1);
} 



/* class should have the right alignment to prevent cache trashing */
class AtomicCounter
{
private:
  atomic_t m_counter;
  char dummy[64-sizeof(atomic_t)]; // (iw) to make sure it's the only
                                   // counter sitting in its
                                   // cacheline....
public:

  AtomicCounter() {
    reset();
  }

  AtomicCounter(const int v) {
    m_counter = v;
  }

  inline void reset() {
    m_counter = -1;
  }

  inline int inc() {
    return atomic_inc(&m_counter);
  }

  inline int dec() {
    return atomic_dec(&m_counter);
  }

  inline int add(const int i) {
    return atomic_add(&m_counter,i);
  }


};

#define DBG_THREAD(x) 


class Barrier
{
protected:
  pthread_barrier_t m_barrier;
public:
  void init(int count)
  {
    pthread_barrier_init(&m_barrier,NULL,count);
  }
  void wait()
  {
    pthread_barrier_wait(&m_barrier);
  }
};


class MultiThreadedTaskQueueServer; /// Actually executes tasks
class MultiThreadedScheduler;       /// Allows suspending/resuming threads
class MultiThreadedTaskQueue        /// Externally visible
{
public:

  friend class MultiThreadedTaskQueueServer;

  enum {
    THREAD_EXIT,
    THREAD_RUNNING
  };

  /// Allows defining jobs in derived classes.
  virtual int task(int jobID, int threadID) {
    /* nothing to do */
    //PING;
    //cout << "THIS SHOULD NOT BE CALLED" << endl;
    return THREAD_RUNNING;
  }

  MultiThreadedTaskQueue() : m_threads(0) {}
  explicit MultiThreadedTaskQueue(int nt) : m_threads(nt) { if (nt) createThreads(nt); }
  ~MultiThreadedTaskQueue() {}

  /// Client differentiator: by default it is mapped to this pointer.
  /// Derived classes could use it to implement
  /// multi-phase processing.
  virtual long long int tag() const {
    return (long long int)this;
  }

  /// Set max # of threads handled by m_server;
  /// actual number executed inside this class (as defined by createThreads)
  /// may be different.
  static void setMaxNumberOfThreads(int threads);
  static int  maxNumberOfThreads();

  /// Request nthreads from server and execute task() in each
  /// (only after startThreads() is called).
  /// startThreads/waitForAllThreads pairs could be executed multiple times.
  void createThreads(int nthreads);
  /// Go!
  virtual void startThreads();
  /// Wait...
  virtual void waitForAllThreads();
  /// Start & wait
  void executeAllThreads() {
    startThreads();
    waitForAllThreads();
  }

  inline int numberOfThreads() const { return m_threads; }

protected:

  volatile int m_assigned_jobs; // # of assigned jobs for client, not used for server
  volatile int m_finished_jobs; // # of completed jobs for client, marker for server
  int m_threads;                // # of jobs; fo reach job task() will be called with job id

  static MultiThreadedTaskQueueServer m_server;
  static MultiThreadedTaskQueue**     m_client;
  static MultiThreadedScheduler*      m_scheduler;

};

/// This class just allows suspending/resuming threads;
/// it is implemented on top of pthreads.
class MultiThreadedSyncPrimitive
{
public:
  MultiThreadedSyncPrimitive() {
    pthread_mutex_init(&m_mutex, NULL);
    pthread_cond_init(&m_cond, NULL);
  }
  ~MultiThreadedSyncPrimitive() {
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
  }

  inline int  lock()      { return pthread_mutex_lock(&m_mutex);         }
  inline int  trylock()   { return pthread_mutex_trylock(&m_mutex);      }
  inline int  unlock()    { return pthread_mutex_unlock(&m_mutex);       }
  inline int  suspend()   { return pthread_cond_wait(&m_cond, &m_mutex); }
  inline void resume()    { pthread_cond_signal(&m_cond);                }
  inline void resumeAll() { pthread_cond_broadcast(&m_cond);             }

protected:
  pthread_mutex_t m_mutex;
  pthread_cond_t  m_cond;
};


class Context : public MultiThreadedTaskQueue {

private:
  RCBProc rcb;
  //int m_threads;
  int m_threadsCreated;
  
public:
  Context() {
    m_threads = 1;
    m_threadsCreated = false;
  } 

  void setRCBProc(RCBProc proc) {
    rcb = proc;
  }

  RCBProc getRCBProc() {
    return rcb;
  }

  
  int getNumberThreads() {
    return m_threads;
  }

  void setNumThreads(int n_threads) {
    m_threads = n_threads;
  }

  int getNumThreads() {
    return m_threads;
  }

  bool threadsCreated() {
    return m_threadsCreated;
  }

  void setThreadsCreated(bool thrCreated) {
    m_threadsCreated = thrCreated;
  }

  virtual int task(int jobID, int threadID);
};

#endif
