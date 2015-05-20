#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "30001" // the port client will be connecting to 
#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int ConnectToClient(char *hostname)
{

}

int main(int argc, char* argv[])
{
	int server_fd, client_fd;
	int numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char remoteIP[INET6_ADDRSTRLEN];

	char msg[MAXDATASIZE];
	char *serv_hostname, *client_hostname;
	int command, ready;

	if (argc != 2)
	{
		fprintf(stderr, "usage: ./client hostname");
		exit(1);
	}
	serv_hostname = argv[1];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(serv_hostname, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("client: socket");
			continue;
		}

		if (connect(server_fd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(server_fd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
		remoteIP, sizeof remoteIP);
	printf("client: connecting to %s\n", remoteIP);

	freeaddrinfo(servinfo);


	while(1)
	{
		// Receive - 0:(wait)
		//			 1:[hostname]
		//			 2:[message]
		if ((numbytes = recv(server_fd, buf, MAXDATASIZE-1, 0)) == -1)
		{
			perror("recv");
			exit(1);
		}
		buf[numbytes] = '\0';

		command = atoi(strtok(buf, ":"));

		switch (command)
		{
		case 0:	// Wait
			ready = 0;
			printf("Waiting for Bruce Schneier...")
			break;
		case 1:	// Get hostname
			ready = 1;
			client_hostname = strtok(NULL, "/0");
			client_fd = ConnectToClient(client_hostname);
			break;
		case 2:	// Receive message
			ready = 1;
			msg = strtok(NULL, "/0");
			printf("RECEIVED: %s\n", msg);
			break;
		default:
			fprintf(stderr, "Something weird happened...")
			break;
		}

		if (ready == 1)
		{
		//Generate random data
		//Send
		//Receive
		//Select (replace scanf)
		//XOR
			printf(">>");
			scanf("%s", msg);
		//printf("Sending %s\n", msg);

			if ((numbytes = send(server_fd, msg, strlen(msg), 0)) == -1)
			{
				perror("send");
				exit(1);
			}
		}
	}
	return 0;
}
































