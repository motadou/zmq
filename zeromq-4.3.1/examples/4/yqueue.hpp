#ifndef __ZMQ_YQUEUE_HPP_INCLUDED__
#define __ZMQ_YQUEUE_HPP_INCLUDED__

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include "atomic_ptr.hpp"

namespace zmq
{

template <typename T, int N> class yqueue_t

{
public:
    inline yqueue_t ()
    {
        _begin_chunk = allocate_chunk();
        _begin_pos   = 0;
        _back_chunk  = NULL;
        _back_pos    = 0;
        _end_chunk   = _begin_chunk;
        _end_pos     = 0;
    }

    inline ~yqueue_t ()
    {
        while (true) 
        {
            if (_begin_chunk == _end_chunk) 
            {
                free (_begin_chunk);
                break;
            }

            chunk_t *o   = _begin_chunk;
            _begin_chunk = _begin_chunk->next;
            free (o);
        }

        chunk_t *sc = _spare_chunk.xchg (NULL);
        free (sc);
    }

    inline T &front () { return _begin_chunk->values[_begin_pos]; }

    inline T &back ()  { return _back_chunk->values[_back_pos];   }

    inline void push ()
    {
        _back_chunk = _end_chunk;
        _back_pos   = _end_pos;

        if (++_end_pos != N)
            return;

        chunk_t *sc = _spare_chunk.xchg(NULL);
        if (sc) 
        {
            _end_chunk->next = sc;
            sc->prev = _end_chunk;
        }
        else 
        {
            _end_chunk->next = allocate_chunk ();
            _end_chunk->next->prev = _end_chunk;
        }

        _end_chunk = _end_chunk->next;
        _end_pos   = 0;
    }

    inline void unpush ()
    {
        if (_back_pos)
        {
            --_back_pos;
        }
        else 
        {
            _back_pos   = N - 1;
            _back_chunk = _back_chunk->prev;
        }

        if (_end_pos)
        {
            --_end_pos;
        }
        else
        {
            _end_pos   = N - 1;
            _end_chunk = _end_chunk->prev;
            free (_end_chunk->next);
            _end_chunk->next = NULL;
        }
    }

    inline void pop ()
    {
        if (++_begin_pos == N) 
        {
            chunk_t * o        = _begin_chunk;
            _begin_chunk       = _begin_chunk->next;
            _begin_chunk->prev = NULL;
            _begin_pos         = 0;

            chunk_t *cs = _spare_chunk.xchg(o);
            free (cs);
        }
    }

private:
    struct chunk_t
    {
        T         values[N];
        chunk_t * prev;
        chunk_t * next;
    };

    inline chunk_t *allocate_chunk ()
    {
        return (chunk_t *) malloc (sizeof (chunk_t));
    }

    chunk_t * _begin_chunk;
    int       _begin_pos;
    chunk_t * _back_chunk;
    int       _back_pos;
    chunk_t * _end_chunk;
    int       _end_pos;

    atomic_ptr_t<chunk_t> _spare_chunk;
};

}

#endif
