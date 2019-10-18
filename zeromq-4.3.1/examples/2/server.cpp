#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
 
#define randof(num)  (int) ((float) (num) * random () / (RAND_MAX + 1.0))
 
int main (void)
{
    //  准备上下文和PUB套接字
    void *context   = zmq_init(1);
    void *publisher = zmq_socket(context, ZMQ_PUB);
    zmq_bind (publisher, "tcp://*:5556");
    zmq_bind (publisher, "ipc://weather.ipc");
	
	//  初始化随机数生成器
    srandom ((unsigned) time (NULL));

    while (1) 
    {
        //  生成数据
        int zipcode, temperature, relhumidity;
        zipcode     = randof (100000);
        temperature = randof (215) - 80;
        relhumidity = randof (50) + 10;
 
        //  向所有订阅者发送消息
        char update [20];
        sprintf (update, "%05d %d %d", zipcode, temperature, relhumidity);
        zmq_send(publisher, update, sizeof(update), 0);
    }

    zmq_close (publisher);
    zmq_term (context);
    return 0;
}
