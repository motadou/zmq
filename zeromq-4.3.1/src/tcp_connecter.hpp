#ifndef __TCP_CONNECTER_HPP_INCLUDED__
#define __TCP_CONNECTER_HPP_INCLUDED__

#include "fd.hpp"
#include "own.hpp"
#include "stdint.hpp"
#include "io_object.hpp"

namespace zmq
{
class io_thread_t;
class session_base_t;
struct address_t;

class tcp_connecter_t : public own_t, public io_object_t
{
public:
    //  If 'delayed_start' is true connecter first waits for a while,
    //  then starts connection process.
    tcp_connecter_t (zmq::io_thread_t *io_thread_, zmq::session_base_t *session_, const options_t &options_, address_t *addr_, bool delayed_start_);
    
    ~tcp_connecter_t ();

private:
    //  ID of the timer used to delay the reconnection.
    enum
    {
        reconnect_timer_id = 1,
        connect_timer_id   = 2
    };

    //  Handlers for incoming commands.
    void process_plug ();
    void process_term (int linger_);

    //  Handlers for I/O events.
    void in_event ();
    void out_event ();
    void timer_event (int id_);

    //  Removes the handle from the poller.
    void rm_handle ();

    //  Internal function to start the actual connection establishment.
    void start_connecting ();

    //  Internal function to add a connect timer
    void add_connect_timer ();

    //  Internal function to add a reconnect timer
    void add_reconnect_timer ();

    //  Internal function to return a reconnect backoff delay.
    //  Will modify the current_reconnect_ivl used for next call
    //  Returns the currently used interval
    int get_new_reconnect_ivl ();

    //  Open TCP connecting socket. Returns -1 in case of error,
    //  0 if connect was successful immediately. Returns -1 with
    //  EAGAIN errno if async connect was launched.
    int open ();

    //  Close the connecting socket.
    void close ();

    //  Get the file descriptor of newly created connection. Returns
    //  retired_fd if the connection was unsuccessful.
    fd_t connect ();

    //  Tunes a connected socket.
    bool tune_socket (fd_t fd_);

    //  Address to connect to. Owned by session_base_t.
    address_t *const _addr;

    //  Underlying socket.
    fd_t _s;

    //  Handle corresponding to the listening socket, if file descriptor is
    //  registered with the poller, or NULL.
    handle_t _handle;

    //  If true, connecter is waiting a while before trying to connect.
    const bool _delayed_start;

    //  True iff a timer has been started.
    bool _connect_timer_started;
    bool _reconnect_timer_started;

    //  Reference to the session we belong to.
    zmq::session_base_t *const _session;

    //  Current reconnect ivl, updated for backoff strategy
    int _current_reconnect_ivl;

    // String representation of endpoint to connect to
    std::string _endpoint;

    // Socket
    zmq::socket_base_t *const _socket;

    tcp_connecter_t (const tcp_connecter_t &);
    const tcp_connecter_t &operator= (const tcp_connecter_t &);
};
}

#endif
