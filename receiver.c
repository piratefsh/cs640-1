#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>

#define MAX_LINE 256
#define NUM_ARGS 5

int debug = 1; 

int rport;					//port requester waits on
int sport 	= 5888;			//server port to request from 
char* file_option;			//name of file being requested

char* host 	= "localhost";

//prints error message and dies
int
die(char* err_msg)
{
	fprintf(stderr, err_msg);
	exit(1);
}

//checks for correct number of arguments and assigns them  
//respectively to port and file_option returns -1 if wrong number of args
int 
parse_args(int argc, char * argv[])
{
	if(argc != 5)
		return -1;

	int i;
	for(i = 0; i < argc; i++)
	{
		if(strcmp(argv[i], "-p") == 0)
			rport = atoi(argv[i+1]);

		else if(strcmp(argv[i], "-o") == 0)
			file_option = strdup(argv[i + 1]);
	}
	return 0;
}

int
make_packet(char type, int seq, char* payload, int len)
{
	int header_size = sizeof(char) + 2 * sizeof(int);
	char packet[header_size + len];

	return 0;
}

int
main(int argc, char* argv[])
{
	char* host = "localhost";

	struct hostent* hp;
	struct sockaddr_in sin;
	char buf[MAX_LINE];
	
	int s;
	int len;

	if(parse_args(argc, argv) < 0)
	{
		die("Usage: receiver host");
	}

	if(debug) printf("Listening on port: %d, Requesting file: %s", rport, file_option);

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