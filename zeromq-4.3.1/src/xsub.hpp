#ifndef __ZMQ_XSUB_HPP_INCLUDED__
#define __ZMQ_XSUB_HPP_INCLUDED__

#include "socket_base.hpp"
#include "session_base.hpp"
#include "dist.hpp"
#include "fq.hpp"
#ifdef ZMQ_USE_RADIX_TREE
#include "radix_tree.hpp"
#else
#include "trie.hpp"
#endif

namespace zmq
{
class ctx_t;
class pipe_t;
class io_thread_t;

class xsub_t : public socket_base_t
{
public:
    xsub_t (zmq::ctx_t *parent_, uint32_t tid_, int sid_);
    ~xsub_t ();

protected:
    //  Overrides of functions from socket_base_t.
    void xattach_pipe (zmq::pipe_t *pipe_,
                       bool subscribe_to_all_,
                       bool locally_initiated_);
    int xsend (zmq::msg_t *msg_);
    bool xhas_out ();
    int xrecv (zmq::msg_t *msg_);
    bool xhas_in ();
    void xread_activated (zmq::pipe_t *pipe_);
    void xwrite_activated (zmq::pipe_t *pipe_);
    void xhiccuped (pipe_t *pipe_);
    void xpipe_terminated (zmq::pipe_t *pipe_);

  private:
    //  Check whether the message matches at least one subscription.
    bool match (zmq::msg_t *msg_);

    //  Function to be applied to the trie to send all the subsciptions
    //  upstream.
    static void
    send_subscription (unsigned char *data_, size_t size_, void *arg_);

    //  Fair queueing object for inbound pipes.
    fq_t _fq;

    //  Object for distributing the subscriptions upstream.
    dist_t _dist;

    //  The repository of subscriptions.
#ifdef ZMQ_USE_RADIX_TREE
    radix_tree _subscriptions;
#else
    trie_t _subscriptions;
#endif

    //  If true, 'message' contains a matching message to return on the
    //  next recv call.
    bool _has_message;
    msg_t _message;

    //  If true, part of a multipart message was already received, but
    //  there are following parts still waiting.
    bool _more;

    xsub_t (const xsub_t &);
    const xsub_t &operator= (const xsub_t &);
};
}

#endif
