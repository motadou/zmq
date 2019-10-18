#ifndef __ZMQ_MUTEX_HPP_INCLUDED__
#define __ZMQ_MUTEX_HPP_INCLUDED__

#include "err.hpp"

//  Mutex class encapsulates OS mutex in a platform-independent way.

#ifdef ZMQ_HAVE_WINDOWS

#include "windows.hpp"

namespace zmq
{
class mutex_t
{
  public:
    inline mutex_t () { InitializeCriticalSection (&_cs); }

    inline ~mutex_t () { DeleteCriticalSection (&_cs); }

    inline void lock () { EnterCriticalSection (&_cs); }

    inline bool try_lock ()
    {
        return (TryEnterCriticalSection (&_cs)) ? true : false;
    }

    inline void unlock () { LeaveCriticalSection (&_cs); }

    inline CRITICAL_SECTION *get_cs () { return &_cs; }

  private:
    CRITICAL_SECTION _cs;

    //  Disable copy construction and assignment.
    mutex_t (const mutex_t &);
    void operator= (const mutex_t &);
};
}

#elif defined ZMQ_HAVE_VXWORKS

#include <vxWorks.h>
#include <semLib.h>

namespace zmq
{
class mutex_t
{
  public:
    inline mutex_t ()
    {
        _semId =
          semMCreate (SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE);
    }

    inline ~mutex_t () { semDelete (_semId); }

    inline void lock () { semTake (_semId, WAIT_FOREVER); }

    inline bool try_lock ()
    {
        if (semTake (_semId, NO_WAIT) == OK) {
            return true;
        }
        return false;
    }

    inline void unlock () { semGive (_semId); }

  private:
    SEM_ID _semId;

    // Disable copy construction and assignment.
    mutex_t (const mutex_t &);
    const mutex_t &operator= (const mutex_t &);
};
}

#else

#include <pthread.h>

namespace zmq
{

class mutex_t
{
public:
    inline mutex_t ()
    {
        int rc = pthread_mutexattr_init (&_attr);
        posix_assert (rc);

        rc = pthread_mutexattr_settype (&_attr, PTHREAD_MUTEX_RECURSIVE);
        posix_assert (rc);

        rc = pthread_mutex_init (&_mutex, &_attr);
        posix_assert (rc);
    }

    inline ~mutex_t ()
    {
        int rc = pthread_mutex_destroy (&_mutex);
        posix_assert (rc);

        rc = pthread_mutexattr_destroy (&_attr);
        posix_assert (rc);
    }

    inline void lock ()
    {
        int rc = pthread_mutex_lock (&_mutex);
        posix_assert (rc);
    }

    inline bool try_lock ()
    {
        int rc = pthread_mutex_trylock (&_mutex);
        if (rc == EBUSY)
            return false;

        posix_assert (rc);
        return true;
    }

    inline void unlock ()
    {
        int rc = pthread_mutex_unlock (&_mutex);
        posix_assert (rc);
    }

    inline pthread_mutex_t *get_mutex () { return &_mutex; }

private:
    pthread_mutex_t     _mutex;
    pthread_mutexattr_t _attr;

    // Disable copy construction and assignment.
    mutex_t (const mutex_t &);
    const mutex_t &operator= (const mutex_t &);
};
}

#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace zmq
{

struct scoped_lock_t
{
    scoped_lock_t (mutex_t &mutex_) : _mutex (mutex_) { _mutex.lock (); }

    ~scoped_lock_t () { _mutex.unlock (); }

  private:
    mutex_t &_mutex;

    // Disable copy construction and assignment.
    scoped_lock_t (const scoped_lock_t &);
    const scoped_lock_t &operator= (const scoped_lock_t &);
};


struct scoped_optional_lock_t
{
    scoped_optional_lock_t (mutex_t *mutex_) : _mutex (mutex_)
    {
        if (_mutex != NULL)
            _mutex->lock ();
    }

    ~scoped_optional_lock_t ()
    {
        if (_mutex != NULL)
            _mutex->unlock ();
    }

private:
    mutex_t *_mutex;

    // Disable copy construction and assignment.
    scoped_optional_lock_t (const scoped_lock_t &);
    const scoped_optional_lock_t &operator= (const scoped_lock_t &);
};

}

#endif
