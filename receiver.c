#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include "util.h"
#include <unistd.h>

#define MAX_LINE 512
#define NUM_ARGS 5
#define MAX_TRACKER_LINES 10


int debug = 1; 

char* tracker_filename	= "tracker.txt";
char* file_option;					//name of file being requested
int rport;							//requester port

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

//returns number of requests in tracker file
int
read_tracker(char* filename, request_t* requests)
{
	FILE* fp;
	if((fp = fopen(filename, "r")) < 0)
	{
		die("Error: Could not open tracker file\n");
	}

	char line [MAX_LINE];
	int n = 0;

	if(debug) printf("%s", "Start parsing tracker file\n");

	while((fgets(line, MAX_LINE, fp)) != NULL )
	{
		//just in case there is an empty line
		if(strlen(line) <= 1) 
			break;

		request_t r;
		strcpy(r.filename, strtok(line, " "));
		r.id 		= atoi(strtok(NULL, " "));
		strcpy(r.host, strtok(NULL, " "));
		r.port		= atoi(strtok(NULL, " "));

		memcpy(requests + n, &r, sizeof(request_t));
		if(debug) printf("%s %d %s %d\n", r.filename, r.id, r.host, r.port);
		n++;
	}

	return n;
}

int
read_packet(packet_t* packet)
{
	printf("Received packet:\n");
	header_t h 					= packet->header;
	char payload[h.len];
	memcpy(payload, packet->payload, h.len);

	printf(" type: %c\n sequence: %d\n length: %d\n payload: %s\n\n", h.type, h.seq, h.len, payload);
	
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
do_request(request_t* r)
{
	struct hostent* hp;
	struct sockaddr_in server, recv;
	int s;

	//translate host to peer IP
	hp = gethostbyname(r->host);

	if(debug) printf("Request:\n filename: %s host: %s port: %d id: %d\n", r->filename, r-> host, r->port, r->id );
	if(!hp)
	{
		die("Error: Unknown Host\n");
	}

	//build server address data structure
	bzero((char*) &server, sizeof(server));
	server.sin_family	= AF_INET;
	server.sin_port 	= htons(r->port);
	bcopy(hp->h_addr, (char*) &server.sin_addr, hp->h_length);

	//build recevier address 
	recv.sin_family = AF_INET;
	recv.sin_port	= rport;
	bcopy(hp->h_addr, (char*) &recv.sin_addr, hp->h_length);


	//active open
	if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		die("Error: Failed to open socket\n");

	//bind client to socket
	if ((bind(s, (struct sockaddr *)&recv, sizeof(recv))) < 0)
	{
		die("Error: Failed to bind\n");
	}

	//assemble packet
	packet_t p;

	int seq 	= r->id;
	int len 	= strlen(r->filename) + 1;
	char type 	= 'R';
	char payload [MAX_PAYLOAD];
 
	memcpy(payload, r->filename, len);
	make_packet(&p, type, seq, len, payload);
	
	//send packet
	if(sendto(s, (char*) &p, sizeof(packet_t), 0, (struct sockaddr*) &server, sizeof(server)) < 0)
	{
		die("Error: Fail to send packet\n");
	}

	//set up for response
	packet_t resp;
	int recv_len;
	socklen_t addr_len = sizeof(server);

	//ready file to stuff payload into
	char* filename = strcat(r->filename, "_receiver");
	FILE* fp = make_file(filename);
	printf("Successfully created file %s\n", filename);

	int end = 0;
	while(end == 0)
	{
		//wait for response
		if((recv_len = recvfrom(s, &resp, sizeof(resp), 0, (struct sockaddr*) &server, &addr_len)) < 0)
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

	//close file and socket
	fclose(fp);
	close(s);

	return 0;
}

int
main(int argc, char* argv[])
{

	if(parse_args(argc, argv) < 0)
	{
		die("Usage: -p <receiver port> -o <file option>\n");
	}

	if(debug) printf("Listening on port: %d, Requesting file: %s\n", rport, file_option);

	//read tracker file
	request_t requests [MAX_TRACKER_LINES];
	int num_requests = read_tracker(tracker_filename, requests);

	if(debug) printf("Number of requests in tracker: %d\n", num_requests);

	int i;
	for(i = 0; i < num_requests; i++)
	{
		do_request(&(requests[i]));
	}
	
	
	exit(0);
}
