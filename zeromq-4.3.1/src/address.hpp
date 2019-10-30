#ifndef __ZMQ_ADDRESS_HPP_INCLUDED__
#define __ZMQ_ADDRESS_HPP_INCLUDED__

#include <string>

namespace zmq
{

class ctx_t;
class tcp_address_t;
class udp_address_t;
class ipc_address_t;
class tipc_address_t;

namespace protocol_name
{
static const char inproc[] = "inproc";
static const char tcp[]    = "tcp";
static const char udp[]    = "udp";
static const char ipc[]    = "ipc";
static const char tipc[]   = "tipc";
}

struct address_t
{
    address_t(const std::string &protocol_, const std::string &address_, ctx_t *parent_);

    ~address_t ();

    const std::string protocol;
    const std::string address;
    ctx_t *const parent;

    //  Protocol specific resolved address
    //  All members must be pointers to allow for consistent initialization
    union
    {
        void           * dummy;
        tcp_address_t  * tcp_addr;
        udp_address_t  * udp_addr;
        ipc_address_t  * ipc_addr;
        tipc_address_t * tipc_addr;
    } resolved;

    int to_string (std::string &addr_) const;
};

}

#endif
