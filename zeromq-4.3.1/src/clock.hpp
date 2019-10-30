#ifndef __ZMQ_CLOCK_HPP_INCLUDED__
#define __ZMQ_CLOCK_HPP_INCLUDED__

#include "stdint.hpp"

namespace zmq
{

class clock_t
{
public:
    clock_t ();

    //  CPU's timestamp counter. Returns 0 if it's not available.
    static uint64_t rdtsc();

    //  High precision timestamp.
    static uint64_t now_us();

    //  Low precision timestamp. In tight loops generating it can be
    //  10 to 100 times faster than the high precision timestamp.
    uint64_t now_ms ();

private:
    //  TSC timestamp of when last time measurement was made.
    uint64_t _last_tsc;

    //  Physical time corresponding to the TSC above (in milliseconds).
    uint64_t _last_time;

private:
    clock_t (const clock_t &);
    const clock_t &operator= (const clock_t &);
};
}

#endif
