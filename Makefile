# Makefile

CC = g++ -w
LIBS = -pthread -lncurses


all: server client

server: server.cpp
	g++ -g -o server server.cpp $(LIBS)
 
client: client.cpp
	g++ -g -o client client.cpp $(LIBS)


clean:
	rm -f *.o client
	rm -f *.o server
