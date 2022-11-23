all: server client

server: server.o
	g++ -o server server.o

server.o: server.cpp
	g++ -O3 -c server.cpp -o server.o -std=c++11

client: client.o
	g++ -o client client.o

client.o: client.cpp
	g++ -O3 -c client.cpp -o client.o -std=c++11

clean: 
	rm -f *.o client server 
