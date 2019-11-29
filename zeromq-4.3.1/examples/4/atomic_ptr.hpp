#ifndef __ZMQ_ATOMIC_PTR_HPP_INCLUDED__
#define __ZMQ_ATOMIC_PTR_HPP_INCLUDED__

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <atomic>

namespace zmq
{

template <typename T> class atomic_ptr_t
{
public:
    inline atomic_ptr_t() { _ptr = NULL; }

    inline void set(T *ptr_) { _ptr = ptr_; }

    inline T *xchg (T *val_) 
    {
        return _ptr.exchange(val_, std::memory_order_acq_rel);
    }

    inline T *cas (T *cmp_, T *val_) 
    {
        _ptr.compare_exchange_strong(cmp_, val_, std::memory_order_acq_rel);
        return cmp_;
    }

private:
    std::atomic<T *> _ptr;
};

}

#endif
