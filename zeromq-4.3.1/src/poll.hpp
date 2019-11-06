#ifndef __ZMQ_POLL_HPP_INCLUDED__
#define __ZMQ_POLL_HPP_INCLUDED__

//  poller.hpp decides which polling mechanism to use.
#include "poller.hpp"
#if defined ZMQ_IOTHREAD_POLLER_USE_POLL

#include <poll.h>
#include <stddef.h>
#include <vector>

#include "ctx.hpp"
#include "fd.hpp"
#include "thread.hpp"
#include "poller_base.hpp"

namespace zmq
{
struct i_poll_events;

//  Implements socket polling mechanism using the POSIX.1-2001
//  poll() system call.

class poll_t : public worker_poller_base_t
{
  public:
    typedef fd_t handle_t;

    poll_t (const thread_ctx_t &ctx_);
    ~poll_t ();

    //  "poller" concept.
    //  These methods may only be called from an event callback; add_fd may also be called before start.
    handle_t add_fd (fd_t fd_, zmq::i_poll_events *events_);
    void rm_fd (handle_t handle_);
    void set_pollin (handle_t handle_);
    void reset_pollin (handle_t handle_);
    void set_pollout (handle_t handle_);
    void reset_pollout (handle_t handle_);
    void stop ();

    static int max_fds ();

  private:
    //  Main event loop.
    virtual void loop ();

    void cleanup_retired ();

    struct fd_entry_t
    {
        fd_t index;
        zmq::i_poll_events *events;
    };

    //  This table stores data for registered descriptors.
    typedef std::vector<fd_entry_t> fd_table_t;
    fd_table_t fd_table;

    //  Pollset to pass to the poll function.
    typedef std::vector<pollfd> pollset_t;
    pollset_t pollset;

    //  If true, there's at least one retired event source.
    bool retired;

    poll_t (const poll_t &);
    const poll_t &operator= (const poll_t &);
};

typedef poll_t poller_t;
}

#endif

#endif
