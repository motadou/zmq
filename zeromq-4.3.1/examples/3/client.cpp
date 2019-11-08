#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <atomic>
#include <iostream>

using namespace std;

std::atomic<int *> _ptr;

inline int *cas(int *cmp_, int *val_)
{
    _ptr.compare_exchange_strong(cmp_, val_, std::memory_order_acq_rel);

    std::cout << ">>::" << *cmp_ << std::endl;

    return cmp_;
}

int main(int argc, char *argv[])
{
    int a = 1;
    int b = 2;
    int c = 3;

    int * pa = &a;
    int * pb = &b;
    int * pc = &c;

    _ptr = pa;

    int * pd = cas(pb, pc);

    std::cout << pd << "|" << *pd << std::endl;
    std::cout << _ptr << "|" << _ptr.load() << std::endl;

    std::cout << *(_ptr.load()) << std::endl;
    std::cout <<   a << "|" <<   b << "|" <<   c << std::endl;
    std::cout << *pa << "|" << *pb << "|" << *pc << std::endl;


    return 0;
}
