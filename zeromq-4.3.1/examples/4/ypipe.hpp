#ifndef __ZMQ_YPIPE_HPP_INCLUDED__
#define __ZMQ_YPIPE_HPP_INCLUDED__

#include <assert.h>
#include <stdio.h>
#include "atomic_ptr.hpp"
#include "ypipe_base.hpp"
#include "yqueue.hpp"

namespace zmq
{
template <typename T, int N> class ypipe_t : public ypipe_base_t<T>
{
public:
    inline ypipe_t()
    {
        _queue.push();

        _r = _w = _f = &_queue.back();
        _c.set(&_queue.back());
    }

    inline virtual ~ypipe_t () { }

    inline void write(const T & value_, bool incomplete_)
    {
        _queue.back() = value_;
        _queue.push();

        if (incomplete_ == false)
            _f = &_queue.back();
    }

    inline bool unwrite(T *value_)
    {
        if (_f == &_queue.back())
            return false;
        _queue.unpush();
        *value_ = _queue.back();
        return true;
    }

    inline bool flush()
    {
        if (_w == _f)
            return true;

        if (_c.cas(_w, _f) != _w)
        {
            _c.set(_f);
            _w = _f;
            return false;
        }

        _w = _f;
        return true;
    }

    inline bool check_read()
    {
        if (&_queue.front() != _r && _r)
        {
            return true;
        }

        printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

        _r = _c.cas(&_queue.front(), NULL);

        if (&_queue.front() == _r)
        {
            printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

            return false;
        }

        if (_r == NULL)
        {
            printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);

            return false;
        }

        return true;
    }

    inline bool read (T *value_)
    {
        if (!check_read ())
            return false;

        *value_ = _queue.front();
        _queue.pop ();
        return true;
    }

    inline bool probe(bool (*fn_) (const T &))
    {
        bool rc = check_read();

        return (*fn_) (_queue.front ());
    }

protected:
    yqueue_t<T, N> _queue;

    T *_w;

    T *_r;

    T *_f;

    atomic_ptr_t<T> _c;
};

}

#endif
