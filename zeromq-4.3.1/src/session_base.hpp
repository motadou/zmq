#ifndef __ZMQ_SESSION_BASE_HPP_INCLUDED__
#define __ZMQ_SESSION_BASE_HPP_INCLUDED__

#include <stdarg.h>
#include "own.hpp"
#include "io_object.hpp"
#include "pipe.hpp"
#include "socket_base.hpp"
#include "stream_engine.hpp"

namespace zmq
{
class  io_thread_t;
struct i_engine;
struct address_t;

class session_base_t : public own_t, public io_object_t, public i_pipe_events
{
public:
    //  Create a session of the particular type.
    static session_base_t * create(zmq::io_thread_t *io_thread_, bool active_, zmq::socket_base_t *socket_, const options_t &options_, address_t *addr_);

    //  To be used once only, when creating the session.
    void attach_pipe (zmq::pipe_t *pipe_);

    //  Following functions are the interface exposed towards the engine.
    virtual void reset();
    void flush();
    void rollback();
    void engine_error(zmq::stream_engine_t::error_reason_t reason_);

    //  i_pipe_events interface implementation.
    void read_activated(zmq::pipe_t *pipe_);
    void write_activated(zmq::pipe_t *pipe_);
    void hiccuped(zmq::pipe_t *pipe_);
    void pipe_terminated(zmq::pipe_t *pipe_);

    //  Delivers a message. Returns 0 if successful; -1 otherwise.
    //  The function takes ownership of the message.
    virtual int push_msg(msg_t *msg_);

    int zap_connect ();
    bool zap_enabled ();

    //  Fetches a message. Returns 0 if successful; -1 otherwise.
    //  The caller is responsible for freeing the message when no
    //  longer used.
    virtual int pull_msg (msg_t *msg_);

    //  Receives message from ZAP socket.
    //  Returns 0 on success; -1 otherwise.
    //  The caller is responsible for freeing the message.
    int read_zap_msg (msg_t *msg_);

    //  Sends message to ZAP socket.
    //  Returns 0 on success; -1 otherwise.
    //  The function takes ownership of the message.
    int write_zap_msg (msg_t *msg_);

    socket_base_t *get_socket ();
    const char *get_endpoint () const;

protected:
    session_base_t (zmq::io_thread_t *io_thread_, bool active_, zmq::socket_base_t *socket_, const options_t &options_, address_t *addr_);

    virtual ~session_base_t ();

private:
    void start_connecting (bool wait_);

    typedef own_t *(session_base_t::*connecter_factory_fun_t) (io_thread_t *io_thread, bool wait_);
    typedef std::pair<const std::string, connecter_factory_fun_t> connecter_factory_entry_t;
    static connecter_factory_entry_t _connecter_factories[];
    typedef std::map<std::string, connecter_factory_fun_t> connecter_factory_map_t;
    static connecter_factory_map_t _connecter_factories_map;

    own_t *create_connecter_tipc (io_thread_t *io_thread_, bool wait_);
    own_t *create_connecter_ipc  (io_thread_t *io_thread_, bool wait_);
    own_t *create_connecter_tcp  (io_thread_t *io_thread_, bool wait_);

    typedef void (session_base_t::*start_connecting_fun_t)(io_thread_t *io_thread);
    typedef std::pair<const std::string, start_connecting_fun_t> start_connecting_entry_t;
    static start_connecting_entry_t _start_connecting_entries[];
    typedef std::map<std::string, start_connecting_fun_t> start_connecting_map_t;
    static start_connecting_map_t _start_connecting_map;

    void start_connecting_pgm  (io_thread_t *io_thread_);
    void start_connecting_norm (io_thread_t *io_thread_);
    void start_connecting_udp  (io_thread_t *io_thread_);

    void reconnect ();

    //  Handlers for incoming commands.
    void process_plug ();
    void process_attach (zmq::i_engine *engine_);
    void process_term (int linger_);

    //  i_poll_events handlers.
    void timer_event (int id_);

    //  Remove any half processed messages. Flush unflushed messages.
    //  Call this function when engine disconnect to get rid of leftovers.
    void clean_pipes ();

    //  If true, this session (re)connects to the peer. Otherwise, it's
    //  a transient session created by the listener.
    const bool _active;

    //  Pipe connecting the session to its socket.
    zmq::pipe_t *_pipe;

    //  Pipe used to exchange messages with ZAP socket.
    zmq::pipe_t *_zap_pipe;

    //  This set is added to with pipes we are disconnecting, but haven't yet completed
    std::set<pipe_t *> _terminating_pipes;

    //  This flag is true if the remainder of the message being processed
    //  is still in the in pipe.
    bool _incomplete_in;

    //  True if termination have been suspended to push the pending
    //  messages to the network.
    bool _pending;

    //  The protocol I/O engine connected to the session.
    zmq::i_engine *_engine;

    //  The socket the session belongs to.
    zmq::socket_base_t *_socket;

    //  I/O thread the session is living in. It will be used to plug in
    //  the engines into the same thread.
    zmq::io_thread_t *_io_thread;

    //  ID of the linger timer
    enum
    {
        linger_timer_id = 0x20
    };

    //  True is linger timer is running.
    bool _has_linger_timer;

    //  Protocol and address to use when connecting.
    address_t *_addr;

private:
    session_base_t (const session_base_t &);
    const session_base_t &operator= (const session_base_t &);
};

}

#endif
