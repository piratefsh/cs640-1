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
#include "util.h"


int PORT;
int REQUESTER_PORT;
int RATE;
int SEQ_NUM;
int LEN;

void die(char* s)
{
	perror(s);
	exit(1);
}

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
	
	/* Prepare socket */
	struct sockaddr_in sin_me, sin_you;
	char buf[LEN];
	int recvLen;
	unsigned int youLen = sizeof(sin_you);
	int s;
	
	bzero((char *)&sin_me, sizeof(sin_me));
	sin_me.sin_family = AF_INET;
	sin_me.sin_addr.s_addr = htonl(INADDR_ANY);
	sin_me.sin_port = htons(PORT);

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		die("socket error");
	}

	if ((bind(s, (struct sockaddr *)&sin_me, sizeof(sin_me))) < 0)
	{
		die("bind error");
	}
	
	/* Initiate passive open */
	while(1)
	{
		printf("%s\n", "Waiting for data...");
		/* Wait to receive data */
		if ((recvLen = recvfrom(s, buf, MAX_PACKET_SIZE, 0, (struct sockaddr *)&sin_you, &youLen)) < 0)
		{
			die("recvfrom error");
		}
		
		packet_t* pin = (packet_t*)buf;
		header_t hin = pin->header;
		
		/* Check if this is a request packet, if it's not, ignore and continue */
		if (hin.type != 'R')
		{
			continue;
		}
		
		/* Open the requested file for reading */
		char* payloadIn = pin->payload;
		FILE *fp;
		fp = fopen(payloadIn, "r");
		
		if (fp == NULL)
		{
			printf("Cannot open file %s\n", payloadIn);
			continue;
		}
		
		packet_t pout;
		header_t hout;
		pout.header = hout;
		char payloadOut[MAX_PAYLOAD];
		
		hout.type = 'D';
		int seq= 1;
		int bytesRead = 0;

		/* Build and send the packets */
		while(!feof(fp))
		{
			memset(&payloadOut[0], 0, sizeof(payloadOut));
			bytesRead = fread(payloadOut, sizeof(char), MAX_PAYLOAD, fp);
			hout.len = bytesRead;
			hout.seq = seq;
			memcpy(pout.payload, payloadOut, MAX_PAYLOAD);

			pout.header = hout;
			
			/* Return file to requester in DATA packets */
			if (sendto(s, &pout, sizeof(pout), 0, (struct sockaddr*)&sin_you, youLen) < 0)
			{
				die("DATA sendto error");
			}
			printf("Payload for packet %d: %s\nLength in bytes: %d\n\n-----------------------------------------------\n\n", seq, pout.payload, hout.len);
			seq++;
		}

		/* Send the END packet */
		hout.type = 'E';
		hout.seq = 0;
		hout.len = 0;
		memset(&payloadOut[0], 0, sizeof(payloadOut));
		pout.header = hout;

		if (sendto(s, &pout, sizeof(pout), 0, (struct sockaddr*)&sin_you, youLen) < 0)
		{
			die("END sendto error");
		}

		printf("Sent endpacket\n");
	}
	
	close(s);
	return(0);
}
