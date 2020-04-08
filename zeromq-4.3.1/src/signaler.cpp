#include "precompiled.hpp"
#include "poller.hpp"
#include "polling_util.hpp"

#if   defined ZMQ_POLL_BASED_ON_POLL
#include <poll.h>
#elif defined ZMQ_POLL_BASED_ON_SELECT
#include <sys/select.h>
#endif

#include "signaler.hpp"
#include "likely.hpp"
#include "stdint.hpp"
#include "config.hpp"
#include "err.hpp"
#include "fd.hpp"
#include "ip.hpp"
#include "tcp.hpp"

#include <unistd.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>

///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper to sleep for specific number of milliseconds (or until signal)
static int sleep_ms (unsigned int ms_)
{
    if (ms_ == 0)
        return 0;

    return usleep (ms_ * 1000);
}

// Helper to wait on close(), for non-blocking sockets, until it completes
// If EAGAIN is received, will sleep briefly (1-100ms) then try again, until
// the overall timeout is reached.
//
static int close_wait_ms (int fd_, unsigned int max_ms_ = 2000)
{
    unsigned int ms_so_far         = 0;
    const unsigned int min_step_ms = 1;
    const unsigned int max_step_ms = 100;
    const unsigned int step_ms     = std::min (std::max(min_step_ms, max_ms_/10), max_step_ms);

    int rc = 0; // do not sleep on first attempt
    do 
    {
        if (rc == -1 && errno == EAGAIN) 
        {
            sleep_ms (step_ms);
            ms_so_far += step_ms;
        }
        rc = close(fd_);
    }
    while (ms_so_far < max_ms_ && rc == -1 && errno == EAGAIN);

    return rc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
zmq::signaler_t::signaler_t()
{
    //  Create the socketpair for signaling.
    if (make_fdpair (&_r, &_w) == 0) 
    {
        unblock_socket(_w);
        unblock_socket(_r);
    }

    pid = getpid ();
}

// This might get run after some part of construction failed, leaving one or
// both of _r and _w retired_fd.
zmq::signaler_t::~signaler_t ()
{
#if defined ZMQ_HAVE_EVENTFD
    if (_r == retired_fd)
        return;
    int rc = close_wait_ms (_r);
    errno_assert (rc == 0);
#else
    if (_w != retired_fd) 
    {
        int rc = close_wait_ms (_w);
        errno_assert (rc == 0);
    }
    
    if (_r != retired_fd) 
    {
        int rc = close_wait_ms (_r);
        errno_assert (rc == 0);
    }
#endif
}

zmq::fd_t zmq::signaler_t::get_fd () const
{
    return _r;
}

void zmq::signaler_t::send()
{
    if (unlikely (pid != getpid ())) 
    {
        //printf("Child process %d signaler_t::send returning without sending #1\n", getpid());
        return; // do not send anything in forked child context
    }

#if defined ZMQ_HAVE_EVENTFD
    const uint64_t inc = 1;
    ssize_t sz = write(_w, &inc, sizeof(inc));
    errno_assert (sz == sizeof (inc));
#else
    unsigned char dummy = 0;
    while (true) 
    {
        ssize_t nbytes = ::send (_w, &dummy, sizeof (dummy), 0);
        if (unlikely (nbytes == -1 && errno == EINTR))
            continue;
        if (unlikely (pid != getpid ())) 
        {
            //printf("Child process %d signaler_t::send returning without sending #2\n", getpid());
            errno = EINTR;
            break;
        }

        zmq_assert (nbytes == sizeof dummy);

        break;
    }
#endif
}

int zmq::signaler_t::wait(int timeout_)
{
    if (unlikely (pid != getpid())) 
    {
        // we have forked and the file descriptor is closed. Emulate an interrupt
        // response.
        //printf("Child process %d signaler_t::wait returning simulating interrupt #1\n", getpid());
        errno = EINTR;
        return -1;
    }

#ifdef ZMQ_POLL_BASED_ON_POLL
    // cmake×ßÕâÀï
    struct pollfd pfd;
    pfd.fd     = _r;
    pfd.events = POLLIN;

    const int rc = poll(&pfd, 1, timeout_);
    if (unlikely (rc < 0)) 
    {
        errno_assert (errno == EINTR);
        return -1;
    }

    if (unlikely (rc == 0)) 
    {
        errno = EAGAIN;
        return -1;
    }

    if (unlikely(pid != getpid())) 
    {
        // we have forked and the file descriptor is closed. Emulate an interrupt
        // response.
        //printf("Child process %d signaler_t::wait returning simulating interrupt #2\n", getpid());
        errno = EINTR;
        return -1;
    }
    zmq_assert (rc == 1);
    zmq_assert (pfd.revents & POLLIN);
    return 0;
#elif defined ZMQ_POLL_BASED_ON_SELECT
    optimized_fd_set_t fds(1);
    FD_ZERO (fds.get ());
    FD_SET (_r, fds.get ());
    struct timeval timeout;
    if (timeout_ >= 0) 
    {
        timeout.tv_sec  = timeout_ / 1000;
        timeout.tv_usec = timeout_ % 1000 * 1000;
    }

    int rc = select (_r + 1, fds.get(), NULL, NULL, timeout_ >= 0 ? &timeout : NULL);
    if (unlikely (rc < 0)) 
    {
        errno_assert (errno == EINTR);
        return -1;
    }

    if (unlikely (rc == 0)) 
    {
        errno = EAGAIN;
        return -1;
    }

    zmq_assert (rc == 1);
    return 0;
#else
    #error
#endif
}

void zmq::signaler_t::recv ()
{
//  Attempt to read a signal.
#if defined ZMQ_HAVE_EVENTFD
    uint64_t dummy;
    ssize_t sz = read(_r, &dummy, sizeof (dummy));
    errno_assert (sz == sizeof (dummy));

    //  If we accidentally grabbed the next signal(s) along with the current
    //  one, return it back to the eventfd object.
    if (unlikely (dummy > 1)) 
    {
        const uint64_t inc = dummy - 1;
        ssize_t sz2 = write (_w, &inc, sizeof (inc));
        errno_assert (sz2 == sizeof (inc));
        return;
    }

    zmq_assert (dummy == 1);
#else
    unsigned char dummy;

    ssize_t nbytes = ::recv(_r, &dummy, sizeof(dummy), 0);
    errno_assert (nbytes >= 0);

    zmq_assert (nbytes == sizeof (dummy));
    zmq_assert (dummy == 0);
#endif
}

int zmq::signaler_t::recv_failable()
{
//  Attempt to read a signal.
#if defined ZMQ_HAVE_EVENTFD
    uint64_t dummy;
    ssize_t sz = read(_r, &dummy, sizeof (dummy));
    if (sz == -1) 
    {
        errno_assert (errno == EAGAIN);
        return -1;
    }
    errno_assert (sz == sizeof (dummy));

    //  If we accidentally grabbed the next signal(s) along with the current
    //  one, return it back to the eventfd object.
    if (unlikely (dummy > 1)) 
    {
        const uint64_t inc = dummy - 1;
        ssize_t sz2 = write(_w, &inc, sizeof (inc));
        errno_assert (sz2 == sizeof (inc));
        return 0;
    }

    zmq_assert (dummy == 1);
#else
    unsigned char dummy;

    ssize_t nbytes = ::recv(_r, &dummy, sizeof (dummy), 0);
    if (nbytes == -1) 
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) 
        {
            errno = EAGAIN;
            return -1;
        }

        errno_assert (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR);
    }

    zmq_assert (nbytes == sizeof (dummy));
    zmq_assert (dummy == 0);
#endif
    return 0;
}

bool zmq::signaler_t::valid() const
{
    return _w != retired_fd;
}

void zmq::signaler_t::forked()
{
    //  Close file descriptors created in the parent and create new pair
    close (_r);
    close (_w);
    make_fdpair(&_r, &_w);
}
