#include "ypipe.hpp"
#include <iostream>


int main()
{
    zmq::ypipe_t<int, 2> mPipe;



    mPipe.write(1, false);
    mPipe.write(2, false);
    mPipe.flush();




    int a = 0;
    bool result = false;
    
    result = mPipe.read(&a);
    std::cout << "a1:" << a << "|" << (result?"true":"false") << std::endl;

    result = mPipe.read(&a);
    std::cout << "a2:" << a << "|" << (result ? "true" : "false") << std::endl;

    result = mPipe.read(&a);
    std::cout << "a3:" << a << "|" << (result ? "true" : "false") << std::endl;

    result = mPipe.read(&a);
    std::cout << "a4:" << a << "|" << (result ? "true" : "false") << std::endl;

    return 0;
}




