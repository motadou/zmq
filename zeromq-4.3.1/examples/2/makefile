HOME := /home/kevintian/objs/zeromq-4.3.1
#LIB :=/usr/local/lib/libzmq.a
#LIB :=/usr/local/lib/x86_64-linux-gnu/libzmq.a
LIB :=${HOME}/build/lib/libzmq.a
#LIB :=${HOME}/src/.libs/libzmq.a
#INC :=/usr/local/include
INC := ${HOME}/include

all:client server

client:
	g++ -g -o client client.cpp -I${INC} ${LIB} -lpthread
	
server:
	g++ -g -o server server.cpp -I${INC} ${LIB} -lpthread
	
clean:
	rm client server
	