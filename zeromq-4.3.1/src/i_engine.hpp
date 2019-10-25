#ifndef __ZMQ_I_ENGINE_HPP_INCLUDED__
#define __ZMQ_I_ENGINE_HPP_INCLUDED__

namespace zmq
{
class io_thread_t;

//  Abstract interface to be implemented by various engines.

struct i_engine
{
    virtual ~i_engine () {}

    //  Plug the engine to the session.
    virtual void plug (zmq::io_thread_t *io_thread_, class session_base_t *session_) = 0;

    //  Terminate and deallocate the engine. Note that 'detached'
    //  events are not fired on termination.
    virtual void terminate () = 0;

    //  This method is called by the session to signalise that more
    //  messages can be written to the pipe.
    //  Returns false if the engine was deleted due to an error.
    //  TODO it is probably better to change the design such that the engine
    //  does not delete itself
    virtual bool restart_input () = 0;

    //  This method is called by the session to signalise that there
    //  are messages to send available.
    virtual void restart_output () = 0;

    virtual void zap_msg_available () = 0;

    virtual const char *get_endpoint () const = 0;
};

}

#endif
