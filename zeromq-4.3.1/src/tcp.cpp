#include "precompiled.hpp"
#include "macros.hpp"
#include "ip.hpp"
#include "tcp.hpp"
#include "err.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

int zmq::tune_tcp_socket(fd_t s_)
{
    //  Disable Nagle's algorithm. We are doing data batching on 0MQ level,
    //  so using Nagle wouldn't improve throughput in anyway, but it would
    //  hurt latency.
    int nodelay = 1;
    int rc = setsockopt(s_, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char *>(&nodelay), sizeof(int));
    tcp_assert_tuning_error(s_, rc);
    if (rc != 0)
        return rc;

    return rc;
}

int zmq::set_tcp_send_buffer(fd_t sockfd_, int bufsize_)
{
    const int rc = setsockopt (sockfd_, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char *>(&bufsize_), sizeof bufsize_);

    tcp_assert_tuning_error (sockfd_, rc);

    return rc;
}

int zmq::set_tcp_receive_buffer(fd_t sockfd_, int bufsize_)
{
    const int rc = setsockopt (sockfd_, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char *> (&bufsize_), sizeof bufsize_);
    tcp_assert_tuning_error (sockfd_, rc);
    return rc;
}

int zmq::tune_tcp_keepalives (fd_t s_, int keepalive_, int keepalive_cnt_, int keepalive_idle_, int keepalive_intvl_)
{
    // These options are used only under certain #ifdefs below.
    LIBZMQ_UNUSED (keepalive_);
    LIBZMQ_UNUSED (keepalive_cnt_);
    LIBZMQ_UNUSED (keepalive_idle_);
    LIBZMQ_UNUSED (keepalive_intvl_);

    // If none of the #ifdefs apply, then s_ is unused.
    LIBZMQ_UNUSED (s_);

    //  Tuning TCP keep-alives if platform allows it
    //  All values = -1 means skip and leave it for OS

    if (keepalive_ != -1) 
    {
        int rc = setsockopt (s_, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char *> (&keepalive_), sizeof (int));
        tcp_assert_tuning_error (s_, rc);
        if (rc != 0)
        {
            return rc;
        }

#ifdef ZMQ_HAVE_TCP_KEEPCNT // 该宏有效
        if (keepalive_cnt_ != -1) 
        {
            int rc = setsockopt(s_, IPPROTO_TCP, TCP_KEEPCNT, &keepalive_cnt_, sizeof(int));
            tcp_assert_tuning_error(s_, rc);
            if (rc != 0)
                return rc;
        }
#endif // ZMQ_HAVE_TCP_KEEPCNT

#ifdef ZMQ_HAVE_TCP_KEEPIDLE // 该宏有效
        if (keepalive_idle_ != -1) 
        {
            int rc = setsockopt(s_, IPPROTO_TCP, TCP_KEEPIDLE, &keepalive_idle_, sizeof(int));
            tcp_assert_tuning_error(s_, rc);
            if (rc != 0)
                return rc;
        }
#endif // ZMQ_HAVE_TCP_KEEPIDLE

#ifdef ZMQ_HAVE_TCP_KEEPINTVL   // 该宏有效
        if (keepalive_intvl_ != -1) 
        {
            int rc = setsockopt (s_, IPPROTO_TCP, TCP_KEEPINTVL, &keepalive_intvl_, sizeof(int));
            tcp_assert_tuning_error(s_, rc);
            if (rc != 0)
                return rc;
        }
#endif // ZMQ_HAVE_TCP_KEEPINTVL

    }

    return 0;
}

int zmq::tune_tcp_maxrt(fd_t sockfd_, int timeout_)
{
    if (timeout_ <= 0)
        return 0;

    LIBZMQ_UNUSED (sockfd_);

// FIXME: should be ZMQ_HAVE_TCP_USER_TIMEOUT
#if defined(TCP_USER_TIMEOUT) // 该宏有效
    int rc = setsockopt(sockfd_, IPPROTO_TCP, TCP_USER_TIMEOUT, &timeout_, sizeof(timeout_));
    tcp_assert_tuning_error(sockfd_, rc);
    return rc;
#else
    return 0;
#endif
}

int zmq::tcp_write(fd_t s_, const void *data_, size_t size_)
{
    ssize_t nbytes = send(s_, static_cast<const char *>(data_), size_, 0);

    //  Several errors are OK. When speculative write is being done we may not
    //  be able to write a single byte from the socket. Also, SIGSTOP issued
    //  by a debugging tool can result in EINTR error.
    if (nbytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
    {
        return 0;
    }

    //  Signalise peer failure.
    if (nbytes == -1) 
    {
        errno_assert (errno != EACCES && errno != EBADF && errno != EDESTADDRREQ && errno != EFAULT && errno != EISCONN && errno != EMSGSIZE && errno != ENOMEM && errno != ENOTSOCK && errno != EOPNOTSUPP);

        return -1;
    }

    return static_cast<int>(nbytes);
}

int zmq::tcp_read(fd_t s_, void *data_, size_t size_)
{
    const ssize_t rc = recv(s_, static_cast<char *>(data_), size_, 0);

    //  Several errors are OK. When speculative read is being done we may not
    //  be able to read a single byte from the socket. Also, SIGSTOP issued
    //  by a debugging tool can result in EINTR error.
    if (rc == -1) 
    {
        errno_assert (errno != EBADF && errno != EFAULT && errno != ENOMEM && errno != ENOTSOCK);
        if (errno == EWOULDBLOCK || errno == EINTR)
            errno = EAGAIN;
    }

    return static_cast<int>(rc);
}

void zmq::tcp_assert_tuning_error(zmq::fd_t s_, int rc_)
{
    if (rc_ == 0)
        return;

    //  Check whether an error occurred
    int err = 0;

    socklen_t len = sizeof(err);

    int rc = getsockopt(s_, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&err), &len);

    //  Assert if the error was caused by 0MQ bug.
    //  Networking problems are OK. No need to assert.
    //  Following code should handle both Berkeley-derived socket
    //  implementations and Solaris.
    if (rc == -1)
        err = errno;

    if (err != 0) 
    {
        errno = err;
        errno_assert (errno == ECONNREFUSED || errno == ECONNRESET
                      || errno == ECONNABORTED || errno == EINTR
                      || errno == ETIMEDOUT || errno == EHOSTUNREACH
                      || errno == ENETUNREACH || errno == ENETDOWN
                      || errno == ENETRESET || errno == EINVAL);
    }
}

void zmq::tcp_tune_loopback_fast_path (const fd_t socket_)
{
    LIBZMQ_UNUSED(socket_);
}
