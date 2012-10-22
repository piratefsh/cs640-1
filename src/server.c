#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define SERVER_PORT 5888
#define MAX_PENDING 5
#define MAX_LINE 256

int main(int argc, char* argv[])
{
	struct sockaddr_in sin;
	char buf[MAX_LINE];
	int len;
	int s;
	int new_s;

	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);

	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket error");
		exit(1);
	}

	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
	{
		perror("bind error");
		exit(1);
	}

	listen(s, MAX_PENDING);

	while(1)
	{
		if ((new_s = accept(s, (struct sockaddr *)&sin, (socklen_t*)&len)) < 0)
		{
			perror("accept error");
			exit(1);
		}

		while ((len = recv(new_s, buf, sizeof(buf), 0)) > 0)
		{
			fputs(buf, stdout);
		}

		close(new_s);
	}
}
