#ifndef __ZMQ_ATOMIC_COUNTER_HPP_INCLUDED__
#define __ZMQ_ATOMIC_COUNTER_HPP_INCLUDED__

#include "stdint.hpp"
#include "macros.hpp"

#if   defined ZMQ_FORCE_MUTEXES
    #define ZMQ_ATOMIC_COUNTER_MUTEX
#elif defined ZMQ_HAVE_ATOMIC_INTRINSICS
    #define ZMQ_ATOMIC_COUNTER_INTRINSIC
#elif (defined __cplusplus && __cplusplus >= 201103L) || (defined _MSC_VER && _MSC_VER >= 1900)
    #define ZMQ_ATOMIC_COUNTER_CXX11
#elif (defined __i386__ || defined __x86_64__) && defined __GNUC__
    #define ZMQ_ATOMIC_COUNTER_X86
#elif defined __ARM_ARCH_7A__ && defined __GNUC__
    #define ZMQ_ATOMIC_COUNTER_ARM
#elif (defined ZMQ_HAVE_SOLARIS || defined ZMQ_HAVE_NETBSD || defined ZMQ_HAVE_GNU)
    #define ZMQ_ATOMIC_COUNTER_ATOMIC_H
#elif defined __tile__
    #define ZMQ_ATOMIC_COUNTER_TILE
#else
    #define ZMQ_ATOMIC_COUNTER_MUTEX
#endif

#if   defined ZMQ_ATOMIC_COUNTER_MUTEX
    #include "mutex.hpp"
#elif defined ZMQ_ATOMIC_COUNTER_CXX11
    #include <atomic>
#elif defined ZMQ_ATOMIC_COUNTER_ATOMIC_H   // solaris
    #include <atomic.h>
#elif defined ZMQ_ATOMIC_COUNTER_TILE
    #include <arch/atomic.h>
#endif

namespace zmq
{
//  This class represents an integer that can be incremented/decremented
//  in atomic fashion.
//
//  In zmq::shared_message_memory_allocator a buffer with an atomic_counter_t
//  at the start is allocated. If the class does not align to pointer size,
//  access to pointers in structures in the buffer will cause SIGBUS on
//  architectures that do not allow mis-aligned pointers (eg: SPARC).
//  Force the compiler to align to pointer size, which will cause the object
//  to grow from 4 bytes to 8 bytes on 64 bit architectures (when not using
//  mutexes).
class atomic_counter_t
{
public:
    typedef uint32_t integer_t;

    inline atomic_counter_t (integer_t value_ = 0) ZMQ_NOEXCEPT : _value (value_)
    {

    }

    //  Set counter _value (not thread-safe).
    inline void set (integer_t value_) ZMQ_NOEXCEPT { _value = value_; }

    //  Atomic addition. Returns the old _value.
    inline integer_t add (integer_t increment_) ZMQ_NOEXCEPT
    {
        integer_t old_value;

#if   defined ZMQ_ATOMIC_COUNTER_INTRINSIC
        old_value = __atomic_fetch_add(&_value, increment_, __ATOMIC_ACQ_REL);
#elif defined ZMQ_ATOMIC_COUNTER_CXX11
        old_value = _value.fetch_add(increment_, std::memory_order_acq_rel);
#elif defined ZMQ_ATOMIC_COUNTER_X86
        __asm__ volatile("lock; xadd %0, %1 \n\t"
                         : "=r"(old_value), "=m"(_value)
                         : "0"(increment_), "m"(_value)
                         : "cc", "memory");
#elif defined ZMQ_ATOMIC_COUNTER_MUTEX
        sync.lock ();
        old_value = _value;
        _value += increment_;
        sync.unlock ();
#else
#error atomic_counter is not implemented for this platform
#endif
        return old_value;
    }

    //  Atomic subtraction. Returns false if the counter drops to zero.
    inline bool sub(integer_t decrement_) ZMQ_NOEXCEPT
    {
#if   defined ZMQ_ATOMIC_COUNTER_INTRINSIC // 使用configure来生成makefile文件时，使用这个
        integer_t nv = __atomic_sub_fetch (&_value, decrement_, __ATOMIC_ACQ_REL);
        return nv != 0;
#elif defined ZMQ_ATOMIC_COUNTER_CXX11
        integer_t old = _value.fetch_sub(decrement_, std::memory_order_acq_rel);
        return (old - decrement_) != 0;
#elif defined ZMQ_ATOMIC_COUNTER_X86
        integer_t oldval = -decrement_;
        volatile integer_t *val = &_value;
        __asm__ volatile("lock; xaddl %0,%1" : "=r"(oldval), "=m"(*val) : "0"(oldval), "m"(*val) : "cc", "memory");
        return oldval != decrement_;
#elif defined ZMQ_ATOMIC_COUNTER_MUTEX
        sync.lock ();
        _value -= decrement_;
        bool result = _value ? true : false;
        sync.unlock ();
        return result;
#else
#error atomic_counter is not implemented for this platform
#endif
    }

    inline integer_t get () const ZMQ_NOEXCEPT { return _value; }

private:
#if defined ZMQ_ATOMIC_COUNTER_CXX11
    std::atomic<integer_t> _value;
#else
    volatile integer_t _value;
#endif

#if defined ZMQ_ATOMIC_COUNTER_MUTEX
    mutex_t sync;
#endif

#if !defined ZMQ_ATOMIC_COUNTER_CXX11
    atomic_counter_t (const atomic_counter_t &);
    const atomic_counter_t &operator= (const atomic_counter_t &);
#endif

#if defined(__GNUC__) || defined(__INTEL_COMPILER)                             \
  || (defined(__SUNPRO_C) && __SUNPRO_C >= 0x590)                              \
  || (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x590)
} __attribute__ ((aligned (sizeof (void *))));
#else
};
#endif
}

//  Remove macros local to this file.
#undef ZMQ_ATOMIC_COUNTER_MUTEX
#undef ZMQ_ATOMIC_COUNTER_INTRINSIC
#undef ZMQ_ATOMIC_COUNTER_CXX11
#undef ZMQ_ATOMIC_COUNTER_X86
#undef ZMQ_ATOMIC_COUNTER_ARM
#undef ZMQ_ATOMIC_COUNTER_WINDOWS
#undef ZMQ_ATOMIC_COUNTER_ATOMIC_H
#undef ZMQ_ATOMIC_COUNTER_TILE

#endif
