#ifndef __ZMQ_V2_ENCODER_HPP_INCLUDED__
#define __ZMQ_V2_ENCODER_HPP_INCLUDED__

#include "encoder.hpp"

namespace zmq
{
//  Encoder for 0MQ framing protocol. Converts messages into data stream.
class v2_encoder_t : public encoder_base_t<v2_encoder_t>
{
public:
    v2_encoder_t (size_t bufsize_);

    virtual ~v2_encoder_t ();

private:
    void size_ready ();
    void message_ready ();

    unsigned char _tmp_buf[9];

private:
    v2_encoder_t (const v2_encoder_t &);
    const v2_encoder_t &operator= (const v2_encoder_t &);
};
}

#endif
