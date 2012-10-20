CC = gcc
CFLAGS = -Wall -Werror

all: requester sender
	cp sender ./files/server1
	cp sender ./files/server2

requester: requester.o
	$(CC)  requester.o -o requester

requester.o:
	$(CC) $(CFLAGS) -c requester.c

sender: sender.o
	$(CC) sender.o -o sender

sender.o:
	$(CC) $(CFLAGS) -c sender.c

clean:
	rm -rf *.o