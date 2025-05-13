SERVER_SOURCES=server.c
SUBSCRIBER_SOURCES=subscriber.c
SOURCES=lib/server_lib.c lib/otcpp.c
CFLAGS=-I headers -g
CC = gcc
PORT = 6000
LOCALHOST = 127.0.0.1
CID = 1

.phony: clean

all: server subscriber

server: $(SERVER_SOURCES)
	$(CC) $(CFLAGS) $(SERVER_SOURCES) $(SOURCES) -o server

subscriber: subscriber.c
	$(CC) $(CFLAGS) $(SUBSCRIBER_SOURCES) $(SOURCES) -o subscriber

run_server: server
	./server $(PORT)

run_sub: subscriber
	./subscriber $(CID) $(LOCALHOST) $(PORT)

clean:
	rm -f subscriber
	rm -f server
