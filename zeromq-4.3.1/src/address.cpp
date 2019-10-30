#include "precompiled.hpp"
#include "macros.hpp"
#include "address.hpp"
#include "ctx.hpp"
#include "err.hpp"
#include "tcp_address.hpp"
#include "udp_address.hpp"
#include "ipc_address.hpp"
#include "tipc_address.hpp"

#include <string>
#include <sstream>

zmq::address_t::address_t (const std::string &protocol_, const std::string &address_, ctx_t *parent_) : protocol (protocol_), address (address_), parent (parent_)
{
    resolved.dummy = NULL;
}

zmq::address_t::~address_t ()
{
    if (protocol == protocol_name::tcp) 
    {
        LIBZMQ_DELETE(resolved.tcp_addr);
    } 
    else if (protocol == protocol_name::udp) 
    {
        LIBZMQ_DELETE(resolved.udp_addr);
    }
    else if (protocol == protocol_name::ipc) 
    {
        LIBZMQ_DELETE(resolved.ipc_addr);
    }
    else if (protocol == protocol_name::tipc) 
    {
        LIBZMQ_DELETE(resolved.tipc_addr);
    }
}

int zmq::address_t::to_string (std::string &addr_) const
{
    if (protocol == protocol_name::tcp  && resolved.tcp_addr)
        return resolved.tcp_addr->to_string (addr_);

    if (protocol == protocol_name::udp  && resolved.udp_addr)
        return resolved.udp_addr->to_string (addr_);

    if (protocol == protocol_name::ipc  && resolved.ipc_addr)
        return resolved.ipc_addr->to_string (addr_);

    if (protocol == protocol_name::tipc && resolved.tipc_addr)
        return resolved.tipc_addr->to_string (addr_);

    if (!protocol.empty () && !address.empty()) 
    {
        std::stringstream s;
        s << protocol << "://" << address;
        addr_ = s.str ();
        return 0;
    }

    addr_.clear ();
    return -1;
}
