CC = gcc
CFLAGS = -Wall -Werror

all: receiver server
	cp server ./files/server1
	cp server ./files/server2

receiver: receiver.o
	$(CC)  receiver.o -o receiver

receiver.o:
	$(CC) $(CFLAGS) -c receiver.c

server: server.o
	$(CC) server.o -o server

server.o:
	$(CC) $(CFLAGS) -c server.c

clean:
	rm -rf *.o