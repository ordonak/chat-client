# Makefile

CC = g++ -w
PTHREAD = -pthread

c-server: c-server.cpp
	g++ -g -o c-server c-server.cpp $(PTHREAD)
 
c-client: c-client.cpp
	g++ -g -o c-client c-client.cpp $(PTHREAD)

all:
	$(CC) -g -o c-client.cpp proxy.cpp $(PTHREAD)

clean:
	rm -f *.o c-client
	rm -f *.o c-server
