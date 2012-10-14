#ifndef UTIL
#define UTIL

#define MAX_PACKET_SIZE 5129
#define MAX_PAYLOAD 5120

typedef struct __header_t
{
	char type;
	int seq;
	int len;

} header_t;

typedef struct __packet_t
{
	header_t header;
	char payload[MAX_PAYLOAD];

} packet_t;

//single instance of a request to server
typedef struct __request_t
{
	char filename[512];
	char host[512];
	int id;
	int port;
} request_t;


#endif
