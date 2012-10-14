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
	printf("Received packet:\n");
	header_t h 					= packet->header;
	char payload[h.len];
	memcpy(payload, packet->payload, h.len);

	printf(" type: %d\n sequence: %d\n length: %d\n payload: %s\n\n", h.type, h.seq, h.len, payload);
	
	return 0;
}

int
make_packet(packet_t* p, char type, int seq, int len, char* payload)
{
	//make header
	header_t h;
	h.type = type;
	h.seq = seq;
	h.len = len;

	//make packet
	p->header = h;
	strcpy((char*) p->payload, payload);

	return 0;
}

//appends payload to file. returns number of bytes written
int
write_file(packet_t* p, FILE* fp)
{
	header_t h 	= p->header;
	int len 	= h.len;
	char payload [len];
	memcpy(payload, p->payload, len);

	int written;
	if((written = fwrite(payload, len, sizeof(char), fp)) < 0)
	{
		die("Error: Fail to write to file");
	}
	return written;
}

FILE*
make_file(char* filename)
{
	FILE* fp;

	if((fp = fopen (filename, "w+")) == NULL)
	{
		die("Error: Fail to open file");
	}

	return fp;
}

//returns 1 if p is an END packet, return 0 otherwise
int
is_end_packet(packet_t* p)
{
	header_t h = p->header;
	if(h.type == 'E')
	{
		return 1;
	}

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
		die("Usage: -p <receiver port> -o <host>\n");
	}

	if(debug) printf("Listening on port: %d, Requesting file: %s\n", rport, file_option);

	//translate host to peer IP
	hp = gethostbyname(host);

	if(!hp)
	{
		die("Error: Unknown Host\n");
	}

	//assemble packet
	packet_t p;

	int seq 	= 1;
	int len 	= strlen(file_option) + 1;
	char type 	= 'R';
	char payload [MAX_PAYLOAD];

	memcpy(payload, file_option, len);

	make_packet(&p, type, seq, len, payload);

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

	//set up for response
	packet_t resp;
	int recv_len;
	socklen_t addr_len = sizeof(sin);

	//ready file to stuff payload into
	FILE* fp = make_file("myFile");
	printf("Successfully created file %s\n", file_option);

	int end = 0;
	while(end == 0)
	{
		//wait for response
		if((recv_len = recvfrom(s, &resp, sizeof(resp), 0, (struct sockaddr*) &sin, &addr_len)) < 0)
		{
			die("Error: Fail to receive server response");
		}

		printf("received %d bytes\n", recv_len);

		read_packet(&resp);

		//check if end packet
		end = is_end_packet(&resp);

		//if havent ended, append payload to file
		if(end == 0)
		{
			int bytes_written = write_file(&resp, fp);
			printf("Wrote %d bytes\n", bytes_written);
		}
	}

	printf("End packet received.\n");
	fclose(fp);
	exit(0);
}