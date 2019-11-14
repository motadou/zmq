#include "precompiled.hpp"
#include "macros.hpp"
#include "thread.hpp"
#include "err.hpp"

bool zmq::thread_t::get_started () const
{
    return _started;
}

#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

extern "C" {
static void *thread_routine (void *arg_)
{
    //  Following code will guarantee more predictable latencies as it'll
    //  disallow any signal handling in the I/O thread.
    sigset_t signal_set;
    int rc = sigfillset(&signal_set);
    errno_assert (rc == 0);
    rc = pthread_sigmask(SIG_BLOCK, &signal_set, NULL);
    posix_assert (rc);

    zmq::thread_t *self = (zmq::thread_t *) arg_;
    self->applySchedulingParameters();
    self->_tfn(self->_arg);
    return NULL;
}

}

void zmq::thread_t::start (thread_fn *tfn_, void *arg_)
{
    _tfn = tfn_;
    _arg = arg_;
    int rc = pthread_create(&_descriptor, NULL, thread_routine, this);
    posix_assert (rc);
    _started = true;
}

void zmq::thread_t::stop()
{
    if (_started) 
    {
        int rc = pthread_join(_descriptor, NULL);
        posix_assert (rc);
    }
}

bool zmq::thread_t::is_current_thread () const
{
    return pthread_self() == _descriptor;
}

void zmq::thread_t::setSchedulingParameters(int priority_, int schedulingPolicy_, const std::set<int> &affinity_cpus_)
{
    _thread_priority      = priority_;
    _thread_sched_policy  = schedulingPolicy_;
    _thread_affinity_cpus = affinity_cpus_;
}

void zmq::thread_t::applySchedulingParameters() // to be called in secondary thread context
{
#if defined _POSIX_THREAD_PRIORITY_SCHEDULING && _POSIX_THREAD_PRIORITY_SCHEDULING >= 0 // cmake&configure走这里
    int policy = 0;
    struct sched_param param;

#if _POSIX_THREAD_PRIORITY_SCHEDULING == 0 && defined _SC_THREAD_PRIORITY_SCHEDULING    // 没有走这里
    if (sysconf (_SC_THREAD_PRIORITY_SCHEDULING) < 0) 
    {
        return;
    }
#endif

    int rc = pthread_getschedparam(pthread_self(), &policy, &param);
    posix_assert (rc);

    if (_thread_sched_policy != ZMQ_THREAD_SCHED_POLICY_DFLT) 
    {
        policy = _thread_sched_policy;
    }

    /* Quoting docs:
       "Linux allows the static priority range 1 to 99 for the SCHED_FIFO and
       SCHED_RR policies, and the priority 0 for the remaining policies."
       Other policies may use the "nice value" in place of the priority:
    */
    bool use_nice_instead_priority = (policy != SCHED_FIFO) && (policy != SCHED_RR);

    if (_thread_priority != ZMQ_THREAD_PRIORITY_DFLT) 
    {
        if (use_nice_instead_priority)
            param.sched_priority = 0;                   // this is the only supported priority for most scheduling policies
        else
            param.sched_priority = _thread_priority;    // user should provide a value between 1 and 99
    }

    rc = pthread_setschedparam(pthread_self(), policy, &param);

    posix_assert (rc);

    if (use_nice_instead_priority && _thread_priority != ZMQ_THREAD_PRIORITY_DFLT) 
    {
        // assume the user wants to decrease the thread's nice value
        // i.e., increase the chance of this thread being scheduled: try setting that to
        // maximum priority.
        rc = nice (-20);

        errno_assert (rc != -1);
        // IMPORTANT: EPERM is typically returned for unprivileged processes: that's because
        //            CAP_SYS_NICE capability is required or RLIMIT_NICE resource limit should be changed to avoid EPERM!
    }

#ifdef ZMQ_HAVE_PTHREAD_SET_AFFINITY // 使用configure编译的代码走这里，而camke没有
    if (!_thread_affinity_cpus.empty ()) 
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        for (std::set<int>::const_iterator it = _thread_affinity_cpus.begin(); it != _thread_affinity_cpus.end (); it++) 
        {
            CPU_SET ((int) (*it), &cpuset);
        }

        rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
        posix_assert (rc);
    }
#endif

#endif
}

void zmq::thread_t::setThreadName (const char *name_)
{
    /* The thread name is a cosmetic string, added to ease debugging of
     * multi-threaded applications. It is not a big issue if this value
     * can not be set for any reason (such as Permission denied in some
     * cases where the application changes its EUID, etc.) The value of
     * "int rc" is retained where available, to help debuggers stepping
     * through code to see its value - but otherwise it is ignored.
    */
    if (!name_)
        return;

#if defined(ZMQ_HAVE_PTHREAD_SETNAME_1)
    int rc = pthread_setname_np (name_);
    if (rc)
        return;
#elif defined(ZMQ_HAVE_PTHREAD_SETNAME_2)
    int rc = pthread_setname_np (_descriptor, name_);
    if (rc)
        return;
#elif defined(ZMQ_HAVE_PTHREAD_SETNAME_3)
    int rc = pthread_setname_np (_descriptor, name_, NULL);
    if (rc)
        return;
#elif defined(ZMQ_HAVE_PTHREAD_SET_NAME)
    pthread_set_name_np (_descriptor, name_);
#endif
}
