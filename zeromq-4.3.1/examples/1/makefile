HOME := /home/kevintian/objs/zeromq-4.3.1
#LIB :=/usr/local/lib/libzmq.a
LIB:=/usr/local/lib/x86_64-linux-gnu/libzmq.a
INC :=/usr/local/include

all:client server

client:
	g++ -o client client.cpp -I${INC} ${LIB} -lpthread
	
server:
	g++ -o server server.cpp -I${INC} ${LIB} -lpthread
	
clean:
	rm client server
	