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
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>

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

	int got_port = 0;
	int got_file = 0;

	for(i = 0; i < argc; i++)
	{
		if(strcmp(argv[i], "-p") == 0)
		{
			rport = atoi(argv[i+1]);
			if(rport < 1024 || rport > 65536)
			{
				die("Please enter a port number from 1024 to 65536");
			}
			got_port++;
		}

		else if(strcmp(argv[i], "-o") == 0){
			file_option = strdup(argv[i + 1]);
			got_file++;
		}

	}


	return 0;
}

//comparison function for sorting requests
int
compare_requests(const void* r, const void* s)
{
	int r_id = ((request_t*)r)->id;
	int s_id = ((request_t*)s)->id;

	if(r_id > s_id)
		return 1;
	else if (s_id > r_id)
		return -1;

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

		//if request in tracker is for the file we're requesting
		if(strcmp(r.filename, file_option) == 0)
		{
			memcpy(requests + n, &r, sizeof(request_t));
			if(debug) printf("%s %d %s %d\n", r.filename, r.id, r.host, r.port);
			n++;
		}
	}

	//sort requests by ID
	qsort(requests, n, sizeof(request_t), compare_requests);

	//print sorted requests

	if(debug) printf("\nSorted requests in tracker file\n");
	int i;
	for(i = 0; i < n; i++)
	{
		request_t r = (requests[i]);
		if(debug) printf("%s %d %s %d\n", r.filename, r.id, r.host, r.port);
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

	printf(" type: %c\n sequence: %d\n length: %d\n\n", h.type, h.seq, h.len);
	
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

//appends payload to file
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
	return 0;
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

//print info about packet
void
print_packet_data(struct tm* local_time, int ms, packet_t* resp, char* server_ip)
{

	char payload_4B[5];
	memcpy(payload_4B, resp->payload, 4);
	payload_4B[4] = '\0';
	printf("Time: %d-%d-%d %d:%d:%d.%d\n",local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday, 
		local_time->tm_hour, local_time->tm_min, local_time->tm_sec, ms);
	printf("Sender: %s\nSequence #: %d\nPayload: %s\n\n", server_ip, ntohl((resp->header).seq), payload_4B);
}

int 
do_request(int s, request_t* r, FILE* fp)
{
	struct hostent* hp;
	struct sockaddr_in server;

	//translate server hostname to peer IP
	hp = gethostbyname(r->host);
	
	//build server address data structure
	bzero((char*) &server, sizeof(server));
	server.sin_family	= AF_INET;
	server.sin_port 	= htons(r->port); 
	bcopy(hp->h_addr, (char*) &server.sin_addr, hp->h_length);

	printf("//-------------------START SENDING TO SENDER-------------------//\n");

	if(debug) printf("Request:\n filename: %s host: %s port: %d id: %d\n", r->filename, r-> host, r->port, r->id );
	if(!hp)
	{
		die("Error: Unknown Host\n");
	}

	//assemble packet
	packet_t p;

	int seq 	= 0;
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

	//number of packets received
	int num_packets     = 0;
	int bytes_received  = 0;
	int first_packet    = 1;
	struct timeval start_time, end_time, curr_time;
	struct tm local_time;
	time_t t;

	int end = 0;
	while(end == 0)
	{
		//wait for response
		if((recv_len = recvfrom(s, &resp, sizeof(resp), 0, (struct sockaddr*) &server, &addr_len)) < 0)
		{
			die("Error: Fail to receive server response");
		}

		gettimeofday(&curr_time, NULL);
		t = time(NULL);
		local_time = *localtime(&t);

		//get start time from first packet
		if(first_packet)
		{
			memcpy(&start_time, &curr_time, sizeof(struct timeval));
			first_packet = 0;
		}

		num_packets++;
		bytes_received += (resp.header).len;


		//check if end packet
		end = is_end_packet(&resp);

		//if havent ended, append payload to file
		if(end == 0)
		{
			write_file(&resp, fp);
		}
		//if already ended, log time
		else
		{
			gettimeofday(&end_time, NULL);
		}

		//print packet data
		print_packet_data(&local_time, (int)(curr_time.tv_usec * 1000), &resp, inet_ntoa(server.sin_addr));

	}

	//done receiving
	if(debug) printf("End packet received.\n");

	time_t time_diff	= (end_time.tv_sec * 1000000 + end_time.tv_usec) - (start_time.tv_sec * 1000000 + start_time.tv_usec);

	printf("\nSUMMARY:\nTotal data packets: %d\nTotal data bytes: %d\nDuration: %lds\nAverage packets/s: %.4f\n\n", num_packets, 
		bytes_received, time_diff, num_packets / (float)(time_diff/1000000.0f));
	
	printf("//--------------------DONE SENDING TO SENDER-------------------//\n\n");

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

	//----------read tracker and prepare file---------------------
	request_t requests [MAX_TRACKER_LINES];
	int num_requests = read_tracker(tracker_filename, requests);

	if(debug) printf("Number of requests in tracker: %d\n", num_requests);

	if(num_requests == 0)
	{
		die("Error: No entries in tracker file for file option\n");
	}

	//prepare file to stuff payload into
	FILE* fp = make_file(requests[0].filename);
	printf("Successfully created file %s\n", requests[0].filename);


	//--------------------prepare socket---------------------------
	struct hostent* rhp;
	struct sockaddr_in recv;
	int s;

	//get requester name
	char recv_name[MAX_LINE];
	gethostname(recv_name, sizeof(recv_name));
	rhp = gethostbyname(recv_name);
	printf("Requester name is: %s\n", recv_name);
	
	//build requester address 
	recv.sin_family = AF_INET;
	recv.sin_port	= rport;
	bcopy(rhp->h_addr, (char*) &recv.sin_addr, rhp->h_length);
	printf("Receiver IP: %s\n\n", inet_ntoa(recv.sin_addr));

	//active open
	if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		die("Error: Failed to open socket\n");

	//bind client to socket
	if ((bind(s, (struct sockaddr *)&recv, sizeof(recv))) < 0)
	{
		die("Error: Failed to bind\n");
	}


	//--------------------do requests in tracker------------------
	int i;
	for(i = 0; i < num_requests; i++)
	{
		do_request(s, &(requests[i]), fp);
	}
	
	//done requesting from all servers
	//close socket and file
	close(s);
	fclose(fp);
	
	exit(0);
}
