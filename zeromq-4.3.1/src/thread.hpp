#ifndef __ZMQ_THREAD_HPP_INCLUDED__
#define __ZMQ_THREAD_HPP_INCLUDED__

#include <pthread.h>
#include <set>

namespace zmq
{

typedef void(thread_fn) (void *);

//  Class encapsulating OS thread. Thread initiation/termination is done
//  using special functions rather than in constructor/destructor so that
//  thread isn't created during object construction by accident, causing
//  newly created thread to access half-initialised object. Same applies
//  to the destruction process: Thread should be terminated before object
//  destruction begins, otherwise it can access half-destructed object.

class thread_t
{
public:
    inline thread_t () : _tfn (NULL), _arg(NULL), _started(false), _thread_priority(ZMQ_THREAD_PRIORITY_DFLT), _thread_sched_policy (ZMQ_THREAD_SCHED_POLICY_DFLT)
    {

    }

    //  Creates OS thread. 'tfn' is main thread function. It'll be passed
    //  'arg' as an argument.
    void start (thread_fn *tfn_, void *arg_);

    //  Returns whether the thread was started, i.e. start was called.
    bool get_started () const;

    //  Returns whether the executing thread is the thread represented by the
    //  thread object.
    bool is_current_thread () const;

    //  Waits for thread termination.
    void stop ();

    // Sets the thread scheduling parameters. Only implemented for
    // pthread. Has no effect on other platforms.
    void setSchedulingParameters (int priority_,
                                  int scheduling_policy_,
                                  const std::set<int> &affinity_cpus_);

    // Sets the thread name, 16 characters max including terminating NUL.
    // Only implemented for pthread. Has no effect on other platforms.
    void setThreadName (const char *name_);

    //  These are internal members. They should be private, however then
    //  they would not be accessible from the main C routine of the thread.
    void applySchedulingParameters ();
    thread_fn *_tfn;
    void *_arg;

  private:
    bool _started;

#ifdef ZMQ_HAVE_WINDOWS
    HANDLE _descriptor;
#elif defined ZMQ_HAVE_VXWORKS
    int _descriptor;
    enum
    {
        DEFAULT_PRIORITY = 100,
        DEFAULT_OPTIONS = 0,
        DEFAULT_STACK_SIZE = 4000
    };
#else
    pthread_t _descriptor;
#endif

    //  Thread scheduling parameters.
    int _thread_priority;
    int _thread_sched_policy;
    std::set<int> _thread_affinity_cpus;

    thread_t (const thread_t &);
    const thread_t &operator= (const thread_t &);
};
}

#endif
