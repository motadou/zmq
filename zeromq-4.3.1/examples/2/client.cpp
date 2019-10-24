#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    void *context = zmq_init(1);

    //  创建连接至服务端的套接字
    printf("正在收集气象信息...\n");
    void *subscriber = zmq_socket(context, ZMQ_SUB);
    zmq_connect(subscriber, "tcp://192.168.9.100:5556");

    //  设置订阅信息，默认为纽约，邮编10001
    const char *filter = (argc > 1) ? argv[1] : "10001 ";
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, filter, strlen(filter));

    //  处理100条更新信息
    int update_nbr;
    long total_temp = 0;
    for (update_nbr = 0; update_nbr < 100; update_nbr++) 
    {
        char string[1024] = { 0 };

        zmq_recv(subscriber, string, sizeof(string), 0);
        int zipcode, temperature, relhumidity;
        sscanf(string, "%d %d %d", &zipcode, &temperature, &relhumidity);
        total_temp += temperature;
    }

    printf("地区邮编 '%s' 的平均温度为 %dF\n", filter, (int)(total_temp / update_nbr));

    zmq_close(subscriber);
    zmq_term(context);
    return 0;
}
