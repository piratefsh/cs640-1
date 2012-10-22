#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>

#define SERVER_PORT 5888
#define MAX_LINE 256

int
die(char* err_msg)
{
	fprintf(stderr, err_msg);
	exit(1);
}

int
main(int argc, char* argv[])
{
	//FILE* fp;
	struct hostent* hp;
	struct sockaddr_in sin;
	char* host;
	char buf[MAX_LINE];
	
	int s;
	int len;

	if(argc == 2)
	{
		host = argv[1];
	}

	else
	{
		die("Usage: receiver host");
	}

	//translate host to peer IP
	hp = gethostbyname(host);

	if(!hp)
	{
		die("Error: Unknown Host");
	}

	//build address data structure
	bzero((char*) &sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char*) &sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);

	//active open
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		die("Error: Failed to open socket");

	if (connect (s, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		die("Error: Fail to connect");
	}

	while(fgets(buf, sizeof(buf), stdin))
	{
		buf[MAX_LINE - 1] = '\0';
		len = strlen(buf) + 1;
		send(s, buf, len, 0);
	}

	exit(0);
}