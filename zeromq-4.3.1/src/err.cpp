#include "precompiled.hpp"
#include "err.hpp"
#include "macros.hpp"

const char *zmq::errno_to_string (int errno_)
{
    switch (errno_) 
    {
        case EFSM:
            return "Operation cannot be accomplished in current state";
        case ENOCOMPATPROTO:
            return "The protocol is not compatible with the socket type";
        case ETERM:
            return "Context was terminated";
        case EMTHREAD:
            return "No thread available";
        case EHOSTUNREACH:
            return "Host unreachable";
        default:
            return strerror (errno_);
    }
}

void zmq::zmq_abort(const char *errmsg_)
{
    LIBZMQ_UNUSED (errmsg_);
    print_backtrace();
    abort();
}

#if defined(HAVE_LIBUNWIND) && !defined(__SUNPRO_CC)

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include "mutex.hpp"

void zmq::print_backtrace (void)
{
    static zmq::mutex_t mtx;
    mtx.lock ();
    Dl_info dl_info;
    unw_cursor_t cursor;
    unw_context_t ctx;
    unsigned frame_n = 0;

    unw_getcontext (&ctx);
    unw_init_local (&cursor, &ctx);

    while (unw_step (&cursor) > 0) {
        unw_word_t offset;
        unw_proc_info_t p_info;
        const char *file_name;
        char *demangled_name;
        char func_name[256] = "";
        void *addr;
        int rc;

        if (unw_get_proc_info (&cursor, &p_info))
            break;

        rc = unw_get_proc_name (&cursor, func_name, 256, &offset);
        if (rc == -UNW_ENOINFO)
            strcpy (func_name, "?");

        addr = (void *) (p_info.start_ip + offset);

        if (dladdr (addr, &dl_info) && dl_info.dli_fname)
            file_name = dl_info.dli_fname;
        else
            file_name = "?";

        demangled_name = abi::__cxa_demangle (func_name, NULL, NULL, &rc);

        printf ("#%u  %p in %s (%s+0x%lx)\n", frame_n++, addr, file_name,
                rc ? func_name : demangled_name, (unsigned long) offset);
        free (demangled_name);
    }
    puts ("");

    fflush (stdout);
    mtx.unlock ();
}

#else

void zmq::print_backtrace ()
{

}

#endif
