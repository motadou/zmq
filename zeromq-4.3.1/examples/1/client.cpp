//包含zmq的头文件 
#include <zmq.h>
#include "stdio.h"
#include <pthread.h>

int main(int argc, char * argv[])
{
    void * pCtx = NULL;
    void * pSock = NULL;
    //使用tcp协议进行通信，需要连接的目标机器IP地址为192.168.1.2
    //通信使用的网络端口 为7766 
    const char * pAddr = "tcp://127.0.0.1:7766";

    //创建context 
    if((pCtx = zmq_ctx_new()) == NULL)
    {
        return 0;
    }

    //创建socket 
    if((pSock = zmq_socket(pCtx, ZMQ_DEALER)) == NULL)
    {
        zmq_ctx_destroy(pCtx);
        return 0;
    }

    int iSndTimeout = 5000*1000;// millsecond
    //设置接收超时 
    if (zmq_setsockopt(pSock, ZMQ_RCVTIMEO, &iSndTimeout, sizeof(iSndTimeout)) < 0)
    {
        zmq_close(pSock);
        zmq_ctx_destroy(pCtx);
        return 0;
    }

    //连接目标IP192.168.1.2，端口7766 
    if (zmq_connect(pSock, pAddr) < 0)
    {
        zmq_close(pSock);
        zmq_ctx_destroy(pCtx);
        return 0;
    }

    //循环发送消息 
    for (int n = 0; n < 2; n++)
    {
        static int i = 0;
        //char szMsg[1024] = {0};

        const int iLen = 8192 * 4;

        char * szMsg = new char[iLen];

        snprintf(szMsg, iLen, "hello world : %3d", i++);
        printf("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
        printf("Enter to send...\n");
        if (zmq_send(pSock, szMsg, iLen, 0) < 0)
        {
            fprintf(stderr, "send message faild\n");

            delete[] szMsg;

            continue;
        }
               
        printf("send message : [%s] succeed\n", szMsg);

        zmq_recv(pSock, szMsg, iLen, 0);

        printf("rcv:%s\n", szMsg);

        delete[] szMsg;

        getchar();
    }

    zmq_close(pSock);

    zmq_term(pCtx);

    return 0;
}