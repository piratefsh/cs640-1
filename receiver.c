#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include "util.h"

#define MAX_LINE 256
#define NUM_ARGS 5

int debug = 1; 

int rport;					//port requester waits on
int sport 	= 5999;			//server port to request from 
char* file_option;			//name of file being requested

char* host 	= "localhost";

//prints error message and dies
int
die(char* err_msg)
{
	fprintf(stderr, "%s", err_msg);
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
read_packet(packet_t* packet)
{
	header_t h 					= packet->header;
	char payload[h.len];
	memcpy(payload, packet->payload, h.len);

	printf("type: %d\n sequence: %d\n length: %d\n payload: %s\n", h.type, h.seq, h.len, payload);
	
	return 0;
}


int
main(int argc, char* argv[])
{
	char* host = "localhost";

	struct hostent* hp;
	struct sockaddr_in sin;
	
	int s;

	if(parse_args(argc, argv) < 0)
	{
		die("Usage: receiver host");
	}

	if(debug) printf("Listening on port: %d, Requesting file: %s\n", rport, file_option);

	//translate host to peer IP
	hp = gethostbyname(host);

	if(!hp)
	{
		die("Error: Unknown Host\n");
	}

	//payload
	char* msg = "hello world.";
	char payload [MAX_PAYLOAD];

	//make header
	header_t h;
	h.type = 'R';
	h.seq = 1;
	h.len = strlen(msg) + 1;

	memcpy(payload, msg, h.len);

	//make packet
	packet_t p;
	p.header = h;
	strcpy((char*) p.payload, payload);

	//read packet
	read_packet(&p);


	//build address data structure
	bzero((char*) &sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char*) &sin.sin_addr, hp->h_length);
	sin.sin_port = htons(sport);

	//active open
	if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		die("Error: Failed to open socket\n");

	//send packet
	if(sendto(s, (char*) &p, sizeof(packet_t), 0, (struct sockaddr*) &sin, sizeof(sin)) < 0)
	{
		die("Error: Fail to send packet\n");
	}

	packet_t resp;

	int recv_len;
	socklen_t addr_len = sizeof(sin);

	//wait for response
	if((recv_len = recvfrom(s, &resp, sizeof(resp), 0, (struct sockaddr*) &sin, &addr_len)) < 0)
	{
		die("Error: Fail to receive server response");
	}

	exit(0);
}