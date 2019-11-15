#include "precompiled.hpp"
#include <new>

#include <string>
#include <stdio.h>

#include "tcp_listener.hpp"
#include "stream_engine.hpp"
#include "io_thread.hpp"
#include "session_base.hpp"
#include "config.hpp"
#include "err.hpp"
#include "ip.hpp"
#include "tcp.hpp"
#include "socket_base.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

zmq::tcp_listener_t::tcp_listener_t(io_thread_t *io_thread_, socket_base_t *socket_, const options_t &options_) :
    own_t (io_thread_, options_),
    io_object_t (io_thread_),
    _s (retired_fd),
    _handle (static_cast<handle_t> (NULL)),
    _socket (socket_)
{

}

zmq::tcp_listener_t::~tcp_listener_t()
{
    zmq_assert (_s == retired_fd);
    zmq_assert (!_handle);
}

void zmq::tcp_listener_t::process_plug()
{
    //  Start polling for incoming connections.
    _handle = add_fd(_s);
    set_pollin(_handle);
}

void zmq::tcp_listener_t::process_term(int linger_)
{
    rm_fd (_handle);
    _handle = static_cast<handle_t> (NULL);
    close ();
    own_t::process_term (linger_);
}

// 该对象是TCP的Listen描述符，产生的accept事件。接收新的连接，然后将其加入到IO线程中
void zmq::tcp_listener_t::in_event()
{
    fd_t fd = accept();

    //  If connection was reset by the peer in the meantime, just ignore it.
    //  TODO: Handle specific errors like ENFILE/EMFILE etc.
    if (fd == retired_fd) 
    {
        _socket->event_accept_failed(_endpoint, zmq_errno ());
        return;
    }

    int rc = tune_tcp_socket(fd);
    rc = rc | tune_tcp_keepalives(fd, options.tcp_keepalive, options.tcp_keepalive_cnt, options.tcp_keepalive_idle, options.tcp_keepalive_intvl);
    rc = rc | tune_tcp_maxrt(fd, options.tcp_maxrt);
    if (rc != 0) 
    {
        _socket->event_accept_failed(_endpoint, zmq_errno());
        return;
    }

    //  Create the engine object for this connection.
    stream_engine_t *engine = new (std::nothrow) stream_engine_t(fd, options, _endpoint);
    alloc_assert (engine);

    //  Choose I/O thread to run connecter in. Given that we are already
    //  running in an I/O thread, there must be at least one available.
    io_thread_t *io_thread = choose_io_thread(options.affinity);
    zmq_assert(io_thread);

    //  Create and launch a session object.
    session_base_t *session = session_base_t::create(io_thread, false, _socket, options, NULL);
    errno_assert(session);
    session->inc_seqnum();
    launch_child(session);                  // 发送给IO线程，加入到IO线程中去，目标是session
    send_attach(session, engine, false);    // 发送attache命令，目标是session

    _socket->event_accepted(_endpoint, fd);
}

void zmq::tcp_listener_t::close ()
{
    zmq_assert (_s != retired_fd);

    int rc = ::close (_s);
    errno_assert (rc == 0);

    _socket->event_closed(_endpoint, _s);
    _s = retired_fd;
}

int zmq::tcp_listener_t::get_address (std::string &addr_)
{
    // Get the details of the TCP socket
    struct sockaddr_storage ss;

    socklen_t sl = sizeof (ss);

    int rc = getsockname (_s, reinterpret_cast<struct sockaddr *> (&ss), &sl);

    if (rc != 0) {
        addr_.clear ();
        return rc;
    }

    tcp_address_t addr (reinterpret_cast<struct sockaddr *> (&ss), sl);
    return addr.to_string (addr_);
}

int zmq::tcp_listener_t::set_address (const char *addr_)
{
    //  Convert the textual address into address structure.
    int rc = _address.resolve (addr_, true, options.ipv6);
    if (rc != 0)
        return -1;

    _address.to_string (_endpoint);

    if (options.use_fd != -1) 
    {
        _s = options.use_fd;
        _socket->event_listening (_endpoint, _s);
        return 0;
    }

    //  Create a listening socket.
    _s = open_socket (_address.family (), SOCK_STREAM, IPPROTO_TCP);

    //  IPv6 address family not supported, try automatic downgrade to IPv4.
    if (_s == zmq::retired_fd && _address.family () == AF_INET6 && errno == EAFNOSUPPORT && options.ipv6) 
    {
        rc = _address.resolve (addr_, true, false);
        if (rc != 0)
            return rc;
        _s = open_socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }

    if (_s == retired_fd) 
    {
        return -1;
    }
    make_socket_noninheritable (_s);

    //  On some systems, IPv4 mapping in IPv6 sockets is disabled by default.
    //  Switch it on in such cases.
    if (_address.family () == AF_INET6)
        enable_ipv4_mapping (_s);

    // Set the IP Type-Of-Service for the underlying socket
    if (options.tos != 0)
        set_ip_type_of_service (_s, options.tos);

    // Set the socket to loopback fastpath if configured.
    if (options.loopback_fastpath)
        tcp_tune_loopback_fast_path (_s);

    // Bind the socket to a device if applicable
    if (!options.bound_device.empty ())
        bind_to_device (_s, options.bound_device);

    //  Set the socket buffer limits for the underlying socket.
    if (options.sndbuf >= 0)
        set_tcp_send_buffer(_s, options.sndbuf);

    if (options.rcvbuf >= 0)
        set_tcp_receive_buffer(_s, options.rcvbuf);

    //  Allow reusing of the address.
    int flag = 1;
    rc = setsockopt(_s, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
    errno_assert (rc == 0);

    //  Bind the socket to the network interface and port.
    rc = bind(_s, _address.addr(), _address.addrlen());

    if (rc != 0)
        goto error;

    //  Listen for incoming connections.
    rc = listen(_s, options.backlog);
    if (rc != 0)
        goto error;

    _socket->event_listening(_endpoint, _s);
    return 0;

error:
    int err = errno;
    close ();
    errno = err;
    return -1;
}

zmq::fd_t zmq::tcp_listener_t::accept()
{
    //  The situation where connection cannot be accepted due to insufficient
    //  resources is considered valid and treated by ignoring the connection.
    //  Accept one connection and deal with different failure modes.
    zmq_assert (_s != retired_fd);

    struct sockaddr_storage ss;
    memset (&ss, 0, sizeof (ss));

    socklen_t ss_len = sizeof (ss);

#if defined ZMQ_HAVE_SOCK_CLOEXEC && defined HAVE_ACCEPT4
    fd_t sock = ::accept4(_s, reinterpret_cast<struct sockaddr *> (&ss), &ss_len, SOCK_CLOEXEC);
#else
    fd_t sock = ::accept(_s, reinterpret_cast<struct sockaddr *> (&ss), &ss_len);
#endif

    if (sock == retired_fd) 
    {
        errno_assert (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR || errno == ECONNABORTED || errno == EPROTO || errno == ENOBUFS || errno == ENOMEM || errno == EMFILE || errno == ENFILE);

        return retired_fd;
    }

    make_socket_noninheritable (sock);

    if (!options.tcp_accept_filters.empty ()) 
    {
        bool matched = false;
        for (options_t::tcp_accept_filters_t::size_type i = 0; i != options.tcp_accept_filters.size (); ++i) 
        {
            if (options.tcp_accept_filters[i].match_address(reinterpret_cast<struct sockaddr *> (&ss), ss_len)) 
            {
                matched = true;
                break;
            }
        }

        if (!matched) 
        {
            int rc = ::close (sock);
            errno_assert (rc == 0);

            return retired_fd;
        }
    }

    if (zmq::set_nosigpipe(sock)) 
    {
        int rc = ::close (sock);
        errno_assert (rc == 0);

        return retired_fd;
    }

    // Set the IP Type-Of-Service priority for this client socket
    if (options.tos != 0)
        set_ip_type_of_service (sock, options.tos);

    return sock;
}
