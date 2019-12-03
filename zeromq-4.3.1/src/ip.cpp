#include "precompiled.hpp"
#include "ip.hpp"
#include "err.hpp"
#include "macros.hpp"
#include "config.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#if defined ZMQ_HAVE_EVENTFD
#include <sys/eventfd.h>
#endif

#if defined ZMQ_HAVE_OPENPGM
#include <pgm/pgm.h>
#endif

zmq::fd_t zmq::open_socket (int domain_, int type_, int protocol_)
{
    int rc;

    //  Setting this option result in sane behaviour when exec() functions
    //  are used. Old sockets are closed and don't block TCP ports etc.
#if defined ZMQ_HAVE_SOCK_CLOEXEC
    type_ |= SOCK_CLOEXEC;
#endif

    const fd_t s = socket(domain_, type_, protocol_);

    if (s == retired_fd) 
    {
        return retired_fd;
    }

    make_socket_noninheritable(s);

    //  Socket is not yet connected so EINVAL is not a valid networking error
    rc = zmq::set_nosigpipe(s);
    errno_assert (rc == 0);

    return s;
}

void zmq::unblock_socket(fd_t s_)
{
    int flags = fcntl (s_, F_GETFL, 0);
    if (flags == -1)
        flags = 0;
    int rc = fcntl (s_, F_SETFL, flags | O_NONBLOCK);
    errno_assert(rc != -1);
}

void zmq::enable_ipv4_mapping(fd_t s_)
{
    LIBZMQ_UNUSED (s_);

#if defined IPV6_V6ONLY && !defined ZMQ_HAVE_OPENBSD
    int flag = 0;

    int rc = setsockopt (s_, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char *> (&flag), sizeof (flag));

    errno_assert (rc == 0);
#endif
}

int zmq::get_peer_ip_address(fd_t sockfd_, std::string &ip_addr_)
{
    int rc;
    struct sockaddr_storage ss;

    socklen_t addrlen = sizeof ss;

    rc = getpeername(sockfd_, reinterpret_cast<struct sockaddr *>(&ss), &addrlen);

    if (rc == -1) 
    {
        errno_assert (errno != EBADF && errno != EFAULT && errno != ENOTSOCK);

        return 0;
    }

    char host[NI_MAXHOST];
    rc = getnameinfo(reinterpret_cast<struct sockaddr *> (&ss), addrlen, host, sizeof host, NULL, 0, NI_NUMERICHOST);
    if (rc != 0)
        return 0;

    ip_addr_ = host;

    union
    {
        struct sockaddr sa;
        struct sockaddr_storage sa_stor;
    } u;

    u.sa_stor = ss;
    return static_cast<int> (u.sa.sa_family);
}

void zmq::set_ip_type_of_service(fd_t s_, int iptos_)
{
    int rc = setsockopt (s_, IPPROTO_IP, IP_TOS, reinterpret_cast<char *> (&iptos_), sizeof (iptos_));

#ifdef ZMQ_HAVE_WINDOWS
    wsa_assert (rc != SOCKET_ERROR);
#else
    errno_assert (rc == 0);
#endif

    //  Windows and Hurd do not support IPV6_TCLASS
#if !defined(ZMQ_HAVE_WINDOWS) && defined(IPV6_TCLASS)
    rc = setsockopt (s_, IPPROTO_IPV6, IPV6_TCLASS,
                     reinterpret_cast<char *> (&iptos_), sizeof (iptos_));

    //  If IPv6 is not enabled ENOPROTOOPT will be returned on Linux and
    //  EINVAL on OSX
    if (rc == -1) {
        errno_assert (errno == ENOPROTOOPT || errno == EINVAL);
    }
#endif
}

int zmq::set_nosigpipe (fd_t s_)
{
#ifdef SO_NOSIGPIPE
    //  Make sure that SIGPIPE signal is not generated when writing to a
    //  connection that was already closed by the peer.
    //  As per POSIX spec, EINVAL will be returned if the socket was valid but
    //  the connection has been reset by the peer. Return an error so that the
    //  socket can be closed and the connection retried if necessary.

    int set = 1;
    int rc = setsockopt (s_, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof (int));
    if (rc != 0 && errno == EINVAL)
        return -1;
    errno_assert (rc == 0);
#else
    // 在Linux系统中没有定义SO_NOSIGPIPE

    LIBZMQ_UNUSED (s_);
#endif

    return 0;
}

void zmq::bind_to_device (fd_t s_, std::string &bound_device_)
{
#ifdef ZMQ_HAVE_SO_BINDTODEVICE
    int rc = setsockopt(s_, SOL_SOCKET, SO_BINDTODEVICE, bound_device_.c_str (), bound_device_.length ());

#ifdef ZMQ_HAVE_WINDOWS
    wsa_assert (rc != SOCKET_ERROR);
#else
    errno_assert (rc == 0);
#endif
#else
    LIBZMQ_UNUSED (s_);
    LIBZMQ_UNUSED (bound_device_);
#endif
}

bool zmq::initialize_network ()
{
#if defined ZMQ_HAVE_OPENPGM

    //  Init PGM transport. Ensure threading and timer are enabled. Find PGM
    //  protocol ID. Note that if you want to use gettimeofday and sleep for
    //  openPGM timing, set environment variables PGM_TIMER to "GTOD" and
    //  PGM_SLEEP to "USLEEP".
    pgm_error_t *pgm_error = NULL;
    const bool ok = pgm_init (&pgm_error);
    if (ok != TRUE) {
        //  Invalid parameters don't set pgm_error_t
        zmq_assert (pgm_error != NULL);
        if (pgm_error->domain == PGM_ERROR_DOMAIN_TIME
            && (pgm_error->code == PGM_ERROR_FAILED)) {
            //  Failed to access RTC or HPET device.
            pgm_error_free (pgm_error);
            errno = EINVAL;
            return false;
        }

        //  PGM_ERROR_DOMAIN_ENGINE: WSAStartup errors or missing WSARecvMsg.
        zmq_assert (false);
    }
#endif

#ifdef ZMQ_HAVE_WINDOWS
    //  Intialise Windows sockets. Note that WSAStartup can be called multiple
    //  times given that WSACleanup will be called for each WSAStartup.

    WORD version_requested = MAKEWORD (2, 2);
    WSADATA wsa_data;
    int rc = WSAStartup (version_requested, &wsa_data);
    zmq_assert (rc == 0);
    zmq_assert (LOBYTE (wsa_data.wVersion) == 2
                && HIBYTE (wsa_data.wVersion) == 2);
#endif

    return true;
}

void zmq::shutdown_network ()
{
#if defined ZMQ_HAVE_OPENPGM
    //  Shut down the OpenPGM library.
    if (pgm_shutdown () != TRUE)
        zmq_assert (false);
#endif
}

int zmq::make_fdpair (fd_t *r_, fd_t *w_)
{
#if defined ZMQ_HAVE_EVENTFD
    int flags = 0;

#if defined ZMQ_HAVE_EVENTFD_CLOEXEC
    //  Setting this option result in sane behaviour when exec() functions
    //  are used. Old sockets are closed and don't block TCP ports, avoid
    //  leaks, etc.
    flags |= EFD_CLOEXEC;
#endif

    fd_t fd = eventfd(0, flags);
    if (fd == -1) 
    {
        errno_assert (errno == ENFILE || errno == EMFILE);
        *w_ = *r_ = -1;
        return -1;
    } 
    else 
    {
        *w_ = *r_ = fd;
        return 0;
    }

#else
    // All other implementations support socketpair()
    int sv[2];
    int type = SOCK_STREAM;
    //  Setting this option result in sane behaviour when exec() functions
    //  are used. Old sockets are closed and don't block TCP ports, avoid
    //  leaks, etc.
#if defined ZMQ_HAVE_SOCK_CLOEXEC
    type |= SOCK_CLOEXEC;
#endif
    int rc = socketpair(AF_UNIX, type, 0, sv);
    if (rc == -1) 
    {
        errno_assert (errno == ENFILE || errno == EMFILE);
        *w_ = *r_ = -1;
        return -1;
    } 
    else 
    {
        make_socket_noninheritable(sv[0]);
        make_socket_noninheritable(sv[1]);

        *w_ = sv[0];
        *r_ = sv[1];
        return 0;
    }
#endif
}

void zmq::make_socket_noninheritable (fd_t sock_)
{
#if (!defined ZMQ_HAVE_SOCK_CLOEXEC || !defined HAVE_ACCEPT4) && defined FD_CLOEXEC
    //  If there 's no SOCK_CLOEXEC, let's try the second best option.
    //  Race condition can cause socket not to be closed (if fork happens
    //  between accept and this point).
    const int rc = fcntl (sock_, F_SETFD, FD_CLOEXEC);
    errno_assert (rc != -1);
#else
    LIBZMQ_UNUSED (sock_);
#endif
}
