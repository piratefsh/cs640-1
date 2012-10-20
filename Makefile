CC = gcc
CFLAGS = -Wall -Werror

all: requester sender
	cp sender ./test_files/sender1/
	cp sender ./test_files/sender2/
	cp requester ./test_files/requester/

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