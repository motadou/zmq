#ifndef __ZMQ_XPUB_HPP_INCLUDED__
#define __ZMQ_XPUB_HPP_INCLUDED__

#include <deque>

#include "socket_base.hpp"
#include "session_base.hpp"
#include "mtrie.hpp"
#include "dist.hpp"

namespace zmq
{
class ctx_t;
class msg_t;
class pipe_t;
class io_thread_t;

class xpub_t : public socket_base_t
{
  public:
    xpub_t (zmq::ctx_t *parent_, uint32_t tid_, int sid_);
    ~xpub_t ();

    //  Implementations of virtual functions from socket_base_t.
    void xattach_pipe (zmq::pipe_t *pipe_,
                       bool subscribe_to_all_ = false,
                       bool locally_initiated_ = false);
    int xsend (zmq::msg_t *msg_);
    bool xhas_out ();
    int xrecv (zmq::msg_t *msg_);
    bool xhas_in ();
    void xread_activated (zmq::pipe_t *pipe_);
    void xwrite_activated (zmq::pipe_t *pipe_);
    int xsetsockopt (int option_, const void *optval_, size_t optvallen_);
    void xpipe_terminated (zmq::pipe_t *pipe_);

  private:
    //  Function to be applied to the trie to send all the subscriptions
    //  upstream.
    static void send_unsubscription (zmq::mtrie_t::prefix_t data_,
                                     size_t size_,
                                     xpub_t *self_);

    //  Function to be applied to each matching pipes.
    static void mark_as_matching (zmq::pipe_t *pipe_, xpub_t *arg_);

    //  List of all subscriptions mapped to corresponding pipes.
    mtrie_t _subscriptions;

    //  List of manual subscriptions mapped to corresponding pipes.
    mtrie_t _manual_subscriptions;

    //  Distributor of messages holding the list of outbound pipes.
    dist_t _dist;

    // If true, send all subscription messages upstream, not just
    // unique ones
    bool _verbose_subs;

    // If true, send all unsubscription messages upstream, not just
    // unique ones
    bool _verbose_unsubs;

    //  True if we are in the middle of sending a multi-part message.
    bool _more;

    //  Drop messages if HWM reached, otherwise return with EAGAIN
    bool _lossy;

    //  Subscriptions will not bed added automatically, only after calling set option with ZMQ_SUBSCRIBE or ZMQ_UNSUBSCRIBE
    bool _manual;

    //  Last pipe that sent subscription message, only used if xpub is on manual
    pipe_t *_last_pipe;

    // Pipes that sent subscriptions messages that have not yet been processed, only used if xpub is on manual
    std::deque<pipe_t *> _pending_pipes;

    //  Welcome message to send to pipe when attached
    msg_t _welcome_msg;

    //  List of pending (un)subscriptions, ie. those that were already
    //  applied to the trie, but not yet received by the user.
    std::deque<blob_t> _pending_data;
    std::deque<metadata_t *> _pending_metadata;
    std::deque<unsigned char> _pending_flags;

    xpub_t (const xpub_t &);
    const xpub_t &operator= (const xpub_t &);
};
}

#endif
