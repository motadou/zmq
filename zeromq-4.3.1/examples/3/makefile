HOME := /home/kevintian/objs/zeromq-4.3.1
#LIB :=/usr/local/lib/libzmq.a
#LIB :=/usr/local/lib/x86_64-linux-gnu/libzmq.a
LIB :=${HOME}/build/lib/libzmq.a
#LIB :=${HOME}/src/.libs/libzmq.a
#INC :=/usr/local/include
INC := ${HOME}/include

all:client

client:
	g++ -std=c++11 -g -o client client.cpp -I${INC} ${LIB} -lpthread

clean:
	rm client
	