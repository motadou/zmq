#include "precompiled.hpp"
#include "clock.hpp"
#include "likely.hpp"
#include "config.hpp"
#include "err.hpp"
#include "mutex.hpp"

#include <stddef.h>
#include <sys/time.h>
#include <time.h>

const uint64_t usecs_per_msec = 1000;
const uint64_t usecs_per_sec  = 1000000;
const uint64_t nsecs_per_usec = 1000;

zmq::clock_t::clock_t () : _last_tsc(rdtsc()), _last_time(now_us()/usecs_per_msec)
{

}

uint64_t zmq::clock_t::now_us ()
{
#if defined HAVE_CLOCK_GETTIME && (defined CLOCK_MONOTONIC || defined ZMQ_HAVE_VXWORKS)
    //  Use POSIX clock_gettime function to get precise monotonic time.
    struct timespec tv;

    int rc = clock_gettime(CLOCK_MONOTONIC, &tv);

    // Fix case where system has clock_gettime but CLOCK_MONOTONIC is not supported.
    // This should be a configuration check, but I looked into it and writing an
    // AC_FUNC_CLOCK_MONOTONIC seems beyond my powers.

    if (rc != 0) 
    {
        //  Use POSIX gettimeofday function to get precise time.
        struct timeval tv;
        int rc = gettimeofday (&tv, NULL);
        errno_assert (rc == 0);
        return tv.tv_sec * usecs_per_sec + tv.tv_usec;
    }
    return tv.tv_sec * usecs_per_sec + tv.tv_nsec / nsecs_per_usec;
#else
    //  Use POSIX gettimeofday function to get precise time.
    struct timeval tv;
    int rc = gettimeofday (&tv, NULL);
    errno_assert (rc == 0);
    return tv.tv_sec * usecs_per_sec + tv.tv_usec;
#endif
}

uint64_t zmq::clock_t::now_ms()
{
    uint64_t tsc = rdtsc();

    //  If TSC is not supported, get precise time and chop off the microseconds.
    if (!tsc) 
    {
        return now_us()/usecs_per_msec;
    }

    //  If TSC haven't jumped back (in case of migration to a different
    //  CPU core) and if not too much time elapsed since last measurement,
    //  we can return cached time value.
    if (likely(tsc - _last_tsc <= (clock_precision/2) && tsc >= _last_tsc))
    {
        return _last_time;
    }

    _last_tsc  = tsc;
    _last_time = now_us()/usecs_per_msec;

    return _last_time;
}

uint64_t zmq::clock_t::rdtsc()
{
#if (defined __GNUC__ && (defined __i386__ || defined __x86_64__))
    uint32_t low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return static_cast<uint64_t>(high) << 32 | low;
#else
    struct timespec ts;

    clock_gettime (CLOCK_MONOTONIC, &ts);

    return static_cast<uint64_t> (ts.tv_sec) * nsecs_per_usec * usecs_per_sec + ts.tv_nsec;
#endif
}
