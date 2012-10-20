/*
	sender.c
	Authors: Tyler Burki (UWID: 9032705106) and Sher-Minn Chong (UWID: 9064830251)
	Purpose: UDP file request server
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include "util.h"


int PORT;
int REQUESTER_PORT;
int RATE;
int SEQ_NUM;
int LEN;

/* Method: die
   Purpose: Prints an error message and quits.
   Parameters: s -- The message to print.
*/
void die(char* s)
{
	perror(s);
	exit(1);
}

/*
	Method: main
	Purpose: Performs a passive open and waits for requests for a file.  When a file is requested, fragments the file
			 into packets and sends them to the requester.
	Returns: 0 if successful
*/
int main(int argc, char* argv[])
{
	int i;
	char* argType;
	int arg;
	char* corArgText = "Please run the program with the correct arguments.\n\n-p:  The port number\n-g:  The requester port\n-r:  The rate (# packets/second)\n-q:  The initial sequence of the packet exchange\n-l:  The length of a packet's payload\n";
	char* corPortText = "Please choose a server port number greater than 1024 and less than 65536.\n";
	
	/* Check for the correct number of arguments */
	if (argc != 11)
	{
		die(corArgText);
	}
	
	/* Initialize the server from the command line args */
	for (i = 0; i < argc - 1; i++)
	{
		argType = argv[i];
		arg = atoi(argv[i+1]);
		if (strcmp(argType,"-p") == 0)
		{
			PORT = arg;
			
			/* Check port, needs to be in range 1024<port<65536 */
			if (!(PORT > 1024) || !(PORT < 65536))
			{
				die(corPortText);
			}
		}
		else if (strcmp(argType, "-g") == 0)
		{
			REQUESTER_PORT = arg;
			
			/* Check port, needs to be in range 1024<port<65536 */
			if (!(REQUESTER_PORT > 1024) || !(REQUESTER_PORT < 65536))
			{
				die(corPortText);
			}
		}
		else if (strcmp(argType, "-r") == 0)
		{
			RATE = arg;

			if (RATE <= 0)
			{
				die("Please run again with a rate greater than 0.");
			}
		}
		else if (strcmp(argType, "-q") == 0)
		{
			SEQ_NUM = arg;
		}
		else if (strcmp(argType, "-l") == 0)
		{
			LEN = arg;
		}
		else if ((i % 2) == 1)
		{
			/* An incorrect argument has been entered, print error and quit */
			perror(strcat(argType, " is not a valid argument.  Please retry with the correct arguments.\n"));
			die(corArgText);
		}
	}
	
	struct sockaddr_in sin, sout;
	char buf[LEN];
	int recvLen;
	unsigned int sendLen = sizeof(sout);
	int sock_send, sock_recv;
	
	/* Prepare sender socket */
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(PORT);
	
	/* Get server addr*/
	struct hostent* hp;
	char sender_name[512];
	gethostname(sender_name, sizeof(sender_name));
	hp = gethostbyname(sender_name);
	bcopy(hp->h_addr, (char*) &sin.sin_addr, hp->h_length);

	if ((sock_send = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		die("socket error");
	}
	
	if ((bind(sock_send, (struct sockaddr *)&sin, sizeof(sin))) < 0)
	{
		die("bind error");
	}
	
	/* Prepare receiver socket */
	bzero((char *)&sout, sizeof(sout));
	sout.sin_family = AF_INET;
	sout.sin_addr.s_addr = htonl(INADDR_ANY);
	sout.sin_port = htons(REQUESTER_PORT);

	if ((sock_recv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		die("socket error");
	}
	
	printf("Sender IP: %s\n", inet_ntoa(sin.sin_addr));
	
	
	/* Initiate passive open */
	printf("%s\n", "Waiting for data...");
	/* Wait to receive data */
	if ((recvLen = recvfrom(sock_send, buf, MAX_PACKET_SIZE, 0, (struct sockaddr *)&sout, &sendLen)) < 0)
	{
		die("recvfrom error");
	}
	
	printf("Receiver IP: %s\n", inet_ntoa(sout.sin_addr));
	
	packet_t* pin = (packet_t*)buf;
	header_t hin = pin->header;
	
	/* Check if this is a request packet, if it's not, ignore and continue */
	if (hin.type != 'R')
	{
		die("Invalid packet type");
	}
	
	/* Open the requested file for reading */
	char* payloadIn = pin->payload;
	FILE *fp;
	fp = fopen(payloadIn, "r");
	
	if (fp == NULL)
	{
		printf("Cannot open file %s\n", payloadIn);
		die("");
	}
	
	/* Set up the structure of the return packets */
	packet_t pout;
	header_t hout;
	pout.header = hout;
	char payloadOut[LEN];
	hout.type = 'D';
	int seq = SEQ_NUM;
	
	/* Set up the send rate */
	double waitTime = (1000/(double)RATE);
	struct timespec sleep_spec, rem_spec;
	sleep_spec.tv_sec = 0;
	sleep_spec.tv_nsec = (long)(waitTime * 1000000);
	time_t t;
	struct tm curTime;
	struct timeval ms;
	
	/* Build and send the packets */
	char dispPayload[4];
	int bytesRead = 0;
	while(!feof(fp))
	{
		memset(&payloadOut[0], 0, sizeof(payloadOut));
		memset(&dispPayload[0], 0, sizeof(dispPayload));
		bytesRead = fread(payloadOut, sizeof(char), LEN, fp);
		hout.len = bytesRead;
		hout.seq = htonl(seq);
		memcpy(pout.payload, payloadOut, sizeof(payloadOut));

		pout.header = hout;
		
		/* Return file to requester in DATA packets */
		if (sendto(sock_recv, &pout, sizeof(pout), 0, (struct sockaddr*)&sout, sendLen) < 0)
		{
			die("DATA sendto error");
		}
		
		/* Print packet info */
		t = time(NULL);
		curTime = *localtime(&t);
		gettimeofday(&ms, NULL);
		memcpy(dispPayload, payloadOut, 4);

		printf("Time: %d-%d-%d %d:%d:%d.%d\n",curTime.tm_year + 1900, curTime.tm_mon + 1, curTime.tm_mday, curTime.tm_hour, curTime.tm_min, curTime.tm_sec, (int)(ms.tv_usec * .001));
		printf("Requester: %s\nSequence #: %d\nPayload: %s\n\n",inet_ntoa(sout.sin_addr), seq, dispPayload);
		
		/* Increase the sequence number and wait to send the next packet */
		seq += bytesRead;
		nanosleep(&sleep_spec, &rem_spec);
	}

	/* Send the END packet */
	hout.type = 'E';
	hout.seq = 0;
	hout.len = 0;
	memset(&payloadOut[0], 0, sizeof(payloadOut));
	pout.header = hout;

	if (sendto(sock_recv, &pout, sizeof(pout), 0, (struct sockaddr*)&sout, sendLen) < 0)
	{
		die("END sendto error");
	}

	printf("Sent end packet\n");
	
	close(sock_recv);
	close(sock_send);
	return(0);
}
