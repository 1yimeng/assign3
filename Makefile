man: client.pdf server.pdf
all: server client

client.pdf:
	groff -Tpdf -man client.man >client.pdf

server.pdf:
	groff -Tpdf -man server.man >server.pdf

server: server.o tands.o helper.o
	g++ -o server server.o helper.o tands.o

server.o: server.cpp
	g++ -O3 -c server.cpp -o server.o -std=c++11

client: client.o tands.o helper.o
	g++ -o client client.o helper.o tands.o

client.o: client.cpp
	g++ -O3 -c client.cpp -o client.o -std=c++11

helper.o: helper.cpp
	g++ -O3 -c helper.cpp -o helper.o -std=c++11

tands.o: tands.cpp
	g++ -O3 -c tands.cpp -o tands.o -std=c++11

clean: 
	rm -f *.o client server *.log
