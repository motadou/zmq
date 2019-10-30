HOME := /home/kevintian/objs/zeromq-4.3.1

##############
#INC :=/usr/local/include
#LIB :=/usr/local/lib/libzmq.a

##############
#INC :=/usr/local/include
#LIB :=/usr/local/lib/x86_64-linux-gnu/libzmq.a

##############
#INC := ${HOME}/include
#LIB :=${HOME}/src/.libs/libzmq.a

##############
INC := ${HOME}/include
LIB :=${HOME}/build/lib/libzmq.a

all:client server

client:
	g++ -g -o client client.cpp -I${INC} ${LIB} -lpthread
	
server:
	g++ -g -o server server.cpp -I${INC} ${LIB} -lpthread
	
clean:
	rm client server
	