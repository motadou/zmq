#include "precompiled.hpp"
#include "clock.hpp"
#include "likely.hpp"
#include "config.hpp"
#include "err.hpp"
#include "mutex.hpp"

#include <stddef.h>

#if !defined ZMQ_HAVE_WINDOWS
#include <sys/time.h>
#endif

#if defined HAVE_CLOCK_GETTIME || defined HAVE_GETHRTIME
#include <time.h>
#endif

const uint64_t usecs_per_msec = 1000;
const uint64_t usecs_per_sec  = 1000000;
const uint64_t nsecs_per_usec = 1000;

zmq::clock_t::clock_t () : _last_tsc (rdtsc ()), _last_time (now_us ()/usecs_per_msec)
{

}

uint64_t zmq::clock_t::now_us ()
{
#if defined ZMQ_HAVE_WINDOWS

    //  Get the high resolution counter's accuracy.
    //  While QueryPerformanceFrequency only needs to be called once, since its
    //  value does not change during runtime, we query it here since this is a
    //  static function. It might make sense to cache it, though.
    LARGE_INTEGER ticks_per_second;
    QueryPerformanceFrequency (&ticks_per_second);

    //  What time is it?
    LARGE_INTEGER tick;
    QueryPerformanceCounter (&tick);

    //  Convert the tick number into the number of seconds
    //  since the system was started.
    const double ticks_div =
      static_cast<double> (ticks_per_second.QuadPart) / usecs_per_sec;
    return static_cast<uint64_t> (tick.QuadPart / ticks_div);

#elif defined HAVE_CLOCK_GETTIME && (defined CLOCK_MONOTONIC || defined ZMQ_HAVE_VXWORKS)

    //  Use POSIX clock_gettime function to get precise monotonic time.
    struct timespec tv;

#if defined ZMQ_HAVE_OSX                                                       \
  && __MAC_OS_X_VERSION_MIN_REQUIRED < 101200 // less than macOS 10.12
    int rc = alt_clock_gettime (SYSTEM_CLOCK, &tv);
#else
    int rc = clock_gettime (CLOCK_MONOTONIC, &tv);
#endif
    // Fix case where system has clock_gettime but CLOCK_MONOTONIC is not supported.
    // This should be a configuration check, but I looked into it and writing an
    // AC_FUNC_CLOCK_MONOTONIC seems beyond my powers.
    if (rc != 0) {
#ifndef ZMQ_HAVE_VXWORKS
        //  Use POSIX gettimeofday function to get precise time.
        struct timeval tv;
        int rc = gettimeofday (&tv, NULL);
        errno_assert (rc == 0);
        return tv.tv_sec * usecs_per_sec + tv.tv_usec;
#endif
    }
    return tv.tv_sec * usecs_per_sec + tv.tv_nsec / nsecs_per_usec;

#elif defined HAVE_GETHRTIME

    return gethrtime () / nsecs_per_usec;

#else

    //  Use POSIX gettimeofday function to get precise time.
    struct timeval tv;
    int rc = gettimeofday (&tv, NULL);
    errno_assert (rc == 0);
    return tv.tv_sec * usecs_per_sec + tv.tv_usec;

#endif
}

uint64_t zmq::clock_t::now_ms ()
{
    uint64_t tsc = rdtsc ();

    //  If TSC is not supported, get precise time and chop off the microseconds.
    if (!tsc) 
    {
#ifdef ZMQ_HAVE_WINDOWS
        // Under Windows, now_us is not so reliable since QueryPerformanceCounter
        // does not guarantee that it will use a hardware that offers a monotonic timer.
        // So, lets use GetTickCount when GetTickCount64 is not available with an workaround
        // to its 32 bit limitation.
        return static_cast<uint64_t> ((*my_get_tick_count64) ());
#else
        return now_us () / usecs_per_msec;
#endif
    }

    //  If TSC haven't jumped back (in case of migration to a different
    //  CPU core) and if not too much time elapsed since last measurement,
    //  we can return cached time value.
    if (likely (tsc - _last_tsc <= (clock_precision / 2) && tsc >= _last_tsc))
        return _last_time;

    _last_tsc = tsc;
#ifdef ZMQ_HAVE_WINDOWS
    _last_time = static_cast<uint64_t> ((*my_get_tick_count64) ());
#else
    _last_time = now_us () / usecs_per_msec;
#endif
    return _last_time;
}

uint64_t zmq::clock_t::rdtsc()
{
#if (defined _MSC_VER && (defined _M_IX86 || defined _M_X64))
    return __rdtsc ();
#elif (defined __GNUC__ && (defined __i386__ || defined __x86_64__))
    uint32_t low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return static_cast<uint64_t> (high) << 32 | low;
#elif (defined __SUNPRO_CC && (__SUNPRO_CC >= 0x5100) && (defined __i386 || defined __amd64 || defined __x86_64))
    union
    {
        uint64_t u64val;
        uint32_t u32val[2];
    } tsc;
    asm("rdtsc" : "=a"(tsc.u32val[0]), "=d"(tsc.u32val[1]));
    return tsc.u64val;
#elif defined(__s390__)
    uint64_t tsc;
    asm("\tstck\t%0\n" : "=Q"(tsc) : : "cc");
    return tsc;
#else
    struct timespec ts;
    #if defined ZMQ_HAVE_OSX && __MAC_OS_X_VERSION_MIN_REQUIRED < 101200 // less than macOS 10.12
    alt_clock_gettime (SYSTEM_CLOCK, &ts);
    #else
    clock_gettime (CLOCK_MONOTONIC, &ts);
    #endif
    return static_cast<uint64_t> (ts.tv_sec) * nsecs_per_usec * usecs_per_sec + ts.tv_nsec;
#endif
}
