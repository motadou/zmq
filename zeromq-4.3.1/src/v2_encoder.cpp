#include "precompiled.hpp"
#include "v2_protocol.hpp"
#include "v2_encoder.hpp"
#include "msg.hpp"
#include "likely.hpp"
#include "wire.hpp"

#include <limits.h>

zmq::v2_encoder_t::v2_encoder_t (size_t bufsize_) : encoder_base_t<v2_encoder_t> (bufsize_)
{
    //  Write 0 bytes to the batch and go to message_ready state.
    next_step(NULL, 0, &v2_encoder_t::message_ready, true);
}

zmq::v2_encoder_t::~v2_encoder_t ()
{

}

void zmq::v2_encoder_t::message_ready()
{
    printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

    //  Encode flags.
    unsigned char & protocol_flags = _tmp_buf[0];
    protocol_flags = 0;
    if (in_progress()->flags() & msg_t::more)
        protocol_flags |= v2_protocol_t::more_flag;
    if (in_progress()->size () > UCHAR_MAX)
        protocol_flags |= v2_protocol_t::large_flag;
    if (in_progress()->flags() & msg_t::command)
        protocol_flags |= v2_protocol_t::command_flag;

    //  Encode the message length. For messages less then 256 bytes,
    //  the length is encoded as 8-bit unsigned integer. For larger
    //  messages, 64-bit unsigned integer in network byte order is used.
    const size_t size = in_progress()->size();

    if (unlikely(size > UCHAR_MAX))
    {
        put_uint64(_tmp_buf + 1, size);
        next_step (_tmp_buf, 9, &v2_encoder_t::size_ready, false);
    } 
    else 
    {
        _tmp_buf[1] = static_cast<uint8_t>(size);
        next_step (_tmp_buf, 2, &v2_encoder_t::size_ready, false);
    }
}

void zmq::v2_encoder_t::size_ready()
{
    //  Write message body into the buffer.
    next_step(in_progress()->data(), in_progress()->size(), &v2_encoder_t::message_ready, true);
}
