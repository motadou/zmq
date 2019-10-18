HOME := /home/kevintian/objs/zeromq-4.3.1

all:client server

client:
	g++ -o client client.cpp -I${HOME}/include ${HOME}/build/lib/libzmq.a -lpthread
	
server:
	g++ -o server server.cpp -I${HOME}/include ${HOME}/build/lib/libzmq.a -lpthread
	
clean:
	rm client server
	