#ifndef __ZMQ_TCP_LISTENER_HPP_INCLUDED__
#define __ZMQ_TCP_LISTENER_HPP_INCLUDED__

#include "fd.hpp"
#include "own.hpp"
#include "stdint.hpp"
#include "io_object.hpp"
#include "tcp_address.hpp"

namespace zmq
{
class io_thread_t;
class socket_base_t;

class tcp_listener_t : public own_t, public io_object_t
{
public:
    tcp_listener_t(zmq::io_thread_t *io_thread_, zmq::socket_base_t *socket_, const options_t &options_);
    
    ~tcp_listener_t();

public:
    //  Set address to listen on.
    int set_address (const char *addr_);

    // Get the bound address for use with wildcard
    int get_address (std::string &addr_);

private:
    //  Handlers for incoming commands.
    void process_plug ();
    void process_term (int linger_);

    //  Handlers for I/O events.
    void in_event ();

    //  Close the listening socket.
    void close ();

    //  Accept the new connection. Returns the file descriptor of the
    //  newly created connection. The function may return retired_fd
    //  if the connection was dropped while waiting in the listen backlog
    //  or was denied because of accept filters.
    fd_t accept ();

    //  Address to listen on.
    tcp_address_t _address;

    //  Underlying socket.
    fd_t _s;

    //  Handle corresponding to the listening socket.
    handle_t _handle;

    //  Socket the listener belongs to.
    zmq::socket_base_t *_socket;

    // String representation of endpoint to bind to
    std::string _endpoint;

private:
    tcp_listener_t (const tcp_listener_t &);
    const tcp_listener_t &operator= (const tcp_listener_t &);
};
}

#endif
