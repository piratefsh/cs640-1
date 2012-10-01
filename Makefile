CC = gcc
CFLAGS = -Wall -Werror

all: receiver server

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