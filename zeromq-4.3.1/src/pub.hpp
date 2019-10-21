#ifndef __ZMQ_PUB_HPP_INCLUDED__
#define __ZMQ_PUB_HPP_INCLUDED__

#include "xpub.hpp"

namespace zmq
{
class ctx_t;
class io_thread_t;
class socket_base_t;
class msg_t;

class pub_t : public xpub_t
{
public:
    pub_t (zmq::ctx_t *parent_, uint32_t tid_, int sid_);

    ~pub_t ();

    //  Implementations of virtual functions from socket_base_t.
    void xattach_pipe (zmq::pipe_t *pipe_, bool subscribe_to_all_ = false, bool locally_initiated_ = false);

    int xrecv (zmq::msg_t *msg_);
    
    bool xhas_in ();

private:
    pub_t (const pub_t &);
    const pub_t &operator= (const pub_t &);
};
}

#endif
