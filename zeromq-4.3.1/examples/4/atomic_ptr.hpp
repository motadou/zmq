#ifndef __ZMQ_ATOMIC_PTR_HPP_INCLUDED__
#define __ZMQ_ATOMIC_PTR_HPP_INCLUDED__

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

namespace zmq
{
inline void *atomic_xchg_ptr(void **ptr_, void *const val_)
{
    return __atomic_exchange_n(ptr_, val_, __ATOMIC_ACQ_REL);
}

inline void * atomic_cas(void *volatile *ptr_, void *cmp_, void *val_) 
{
    void *old = cmp_;
    __atomic_compare_exchange_n(ptr_, &old, val_, false, __ATOMIC_RELEASE, __ATOMIC_ACQUIRE);
    return old;
}

template <typename T> class atomic_ptr_t
{
public:
    inline atomic_ptr_t ()  { _ptr = NULL; }

    inline void set (T *ptr_)  { _ptr = ptr_; }

    inline T *xchg (T *val_) 
    {
        return (T *) atomic_xchg_ptr ((void **) &_ptr, val_);
    }

    inline T *cas (T *cmp_, T *val_) 
    {
        return (T *) atomic_cas((void **) &_ptr, cmp_, val_);
    }

private:
    volatile T *_ptr;
};

}

#endif
