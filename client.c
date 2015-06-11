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
#include <pthread.h>

#include <gcrypt.h>
#include <openssl/dh.h>
#include <openssl/bn.h>
#include <openssl/rand.h>

#define PORT "30001" // the port client will be connecting to 
#define CPORT "30002"
#define BACKLOG 10
#define MAXDATASIZE 500 // max number of bytes we can get at once 

typedef struct client
{
    char hostname[INET6_ADDRSTRLEN];
    char port[6];
} CLIENT;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void *StartDH(void* client)
{
	//Send 

	CLIENT* c = (CLIENT*)client;
	int client_fd;
	int rv;
	int numbytes;
	struct addrinfo hints, *servinfo, *p;
	char remoteIP[INET6_ADDRSTRLEN];

	DH *dh;
	int prime_len = 1024;
	int generator = 2;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	printf(" - %s - \n", c->hostname);

	if ((rv = getaddrinfo(c->hostname, c->port, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo(connect): %s\n", gai_strerror(rv));
		exit(1);
	}

	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((client_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("client: socket");
			continue;
		}

		if (connect(client_fd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(client_fd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "client: failed to connect\n");
		exit(1);
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
		remoteIP, sizeof remoteIP);
	printf("client: connecting to %s\n", remoteIP);

	freeaddrinfo(servinfo);

	// Diffie-Hellman
	dh = DH_new();
	DH_generate_parameters_ex(dh, prime_len, generator, NULL);

	DH_free(dh);

	if ((numbytes = send(client_fd, "msg", 3, 0)) == -1)
	{
		perror("send");
		exit(1);
	}

	return NULL;//client_fd;
}

int ConnectToClient(char* sport, char *hostname, char* rport)
{
	//Receive

	pthread_t startThread;
	void *status;
	CLIENT c;

	int rv;
	int yes = 1;
	int listener, new_fd;
	int numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
    socklen_t sin_size;

 	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    printf("	%s, %s, %s", sport, hostname, rport);

    if ((rv = getaddrinfo(NULL, sport, &hints, &servinfo)) != 0) 
    {
    	fprintf(stderr, "getaddrinfo(receive): %s\n", gai_strerror(rv));
    	return 1;
    }

    /* Loop through all the results and bind to the first we can. */
    for(p = servinfo; p != NULL; p = p->ai_next) 
    {
    	if ((listener = socket(p->ai_family, p->ai_socktype,
    		p->ai_protocol)) == -1) 
    	{
    		perror("server: socket");
    		continue;
    	}

    	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) 
        {
            perror("setsockopt");
            return 1;
        }

    	if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) 
    	{
    		close(listener);
    		perror("server: bind");
    		continue;
    	}

    	break;
    }

    if (p == NULL) 
    {
    	fprintf(stderr, "server: failed to bind socket\n");
    	return 2;
    }

    freeaddrinfo(servinfo);

    if (listen(listener, BACKLOG) == -1) 
    {
        perror("listen");
        return 1;
    }

    strcpy(c.hostname, hostname);
    strcpy(c.port, rport);
    rv = pthread_create(&startThread, NULL, StartDH, &c);

    sin_size = sizeof their_addr;
    new_fd = accept(listener, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) 
    {
        perror("accept");
        return 1;
    }

    if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) 
    {
        perror("recv(client)");
        return 1;
    }
    buf[numbytes] = '\0';
    printf(" dh: %s\n", buf);

    close(listener);
    rv = pthread_join(startThread, &status);

	return 0;
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
	char *sport, *rport;
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
		//			 1:[sender port]:[hostname]:[receiver port]
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
			printf("Waiting for Bruce Schneier...\n");
			break;
		case 1:	// Get hostname, ports
			ready = 1;
			sport = strtok(NULL, ":");
			client_hostname = strtok(NULL, ":");
			rport = strtok(NULL, "\0");
			client_fd = ConnectToClient(sport, client_hostname, rport);
			break;
		case 2:	// Receive message
			ready = 1;
			strcpy(msg, strtok(NULL, "\0"));
			printf("RECEIVED: %s\n", msg);
			break;
		default:
			fprintf(stderr, "Something weird happened...\n");
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
































