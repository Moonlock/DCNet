#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <openssl/dh.h>
#include <openssl/bn.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#define PORT "30001" /* The port the server is listening on. */
#define BACKLOG 10
#define MAXMESSAGELENGTH 257 /* Messages are 256 bytes, plus nul byte. */


DH* dh;								/* Diffie-Hellman key. */
unsigned char *SKRight, *SKLeft;	/* Keys user shares with clients on 
									   their 'left' and 'right'. */

typedef struct client
{
    char hostname[INET6_ADDRSTRLEN];
    char port[6];
} CLIENT;


/* Get sockaddr, IPv4 or IPv6. */
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void* StartDH(void* client)
{
	CLIENT* c = (CLIENT*)client;
	BIGNUM* pubKey = NULL;

	int fd;
	struct addrinfo hints, *servinfo, *p;
	char remoteIP[INET6_ADDRSTRLEN];
	char buf[MAXMESSAGELENGTH];
	int rv, numbytes;


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(c->hostname, c->port, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(-1);
	}

	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("client: socket");
			continue;
		}

		if (connect(fd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(fd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "client: failed to connect.\n");
		exit(-1);
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), remoteIP, sizeof remoteIP);
	printf("[DEBUG] Connecting to %s.\n", remoteIP);

	freeaddrinfo(servinfo);

	if ((numbytes = send(fd, BN_bn2hex(dh->pub_key), strlen(BN_bn2hex(dh->pub_key)), 0)) == -1)
	{
		perror("send");
		exit(-1);
	}

	if ((numbytes = recv(fd, buf, MAXMESSAGELENGTH-1, 0)) == -1)
	{
		perror("client: recv");
		exit(-1);
	}

	buf[numbytes] = '\0';
	BN_hex2bn(&pubKey, buf);

	SKRight = OPENSSL_malloc(sizeof(unsigned char) * (DH_size(dh)));
	if(DH_compute_key((unsigned char *)SKRight, pubKey, dh) == -1)
	{
		perror("DH_compute_key");
		exit(-1);
	}

	// TODO: Key extraction.

	close(fd);

	return NULL;
}

int ConnectToClient(char* local_port, char* hostname, char* remote_port)
{
	CLIENT c;
	BIGNUM* pubKey = NULL;

	pthread_t DH_thread;
	void* status;

	int listener, fd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	int yes = 1;

	int rv, numbytes;
	char buf[MAXMESSAGELENGTH];


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (DH_generate_key(dh) == 0)
    {
    	perror("client: DH_generate_key");
    	exit(-1);
    }

    printf("[DEBUG] lp:%s, h:%s, rp:%s\n", local_port, hostname, remote_port);

    if ((rv = getaddrinfo(NULL, local_port, &hints, &servinfo)) != 0)
    {
    	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    	exit(-1);
    }

    /* Loop through all the results and bind to the first we can. */
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
    	if ((listener = socket(p->ai_family, p->ai_socktype, 
    		p->ai_protocol)) == -1)
    	{
    		perror("client: socket");
    		continue;
    	}

    	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    	{
    		perror("client: setsockopt");
    		exit(-1);
    	}

    	if (bind(listener, p->ai_addr, p->ai_addrlen) == -1)
    	{
    		close(listener);
    		perror("client: bind");
    		continue;
    	}

    	break;
    }

    if (p == NULL)
    {
    	fprintf(stderr, "client: failed to bind socket.\n");
    	exit(-1);
    }

    freeaddrinfo(servinfo);

    if (listen(listener, BACKLOG) == -1)
    {
    	perror("client: listen");
    	exit(-1);
    }

    strcpy(c.hostname, hostname);
    strcpy(c.port, remote_port);
    usleep(1000000);
    rv = pthread_create(&DH_thread, NULL, StartDH, &c);

    sin_size = sizeof their_addr;
    fd = accept(listener, (struct sockaddr *)&their_addr, &sin_size);
    if (fd == -1)
    {
    	perror("client: accept");
    	exit(-1);
    }

    if ((numbytes = recv(fd, buf, MAXMESSAGELENGTH-1, 0)) == -1)
    {
    	perror("client: recv");
    	exit(-1);
    }

    if (send(fd, BN_bn2hex(dh->pub_key), strlen(BN_bn2hex(dh->pub_key)), 0) == -1)
    {
    	perror("client: send");
    	exit(-1);
    }

    rv = pthread_join(DH_thread, &status);
    if (rv != 0)
    {
    	perror("client: pthread_join");
    	exit(-1);
    }

    buf[numbytes] = '\0';
    BN_hex2bn(&pubKey, buf);	// TODO: Error checking.

    SKLeft = OPENSSL_malloc(sizeof(unsigned char) * (DH_size(dh)));	
    							// Here too.
    
    if (DH_compute_key((unsigned char *)SKLeft, pubKey, dh) == -1)
    {
    	perror("DH_compute_key");
    	exit(-1);
    }

    // TODO: Key extraction.

    close(listener);
    close(fd);

    return 0;
}

int main(int argc, char const *argv[])
{
	int server_fd, client_fd;
	struct addrinfo hints, *servinfo, *p;
	char remoteIP[INET6_ADDRSTRLEN];
	const char *serv_hostname;
	char *client_hostname;
	char *local_port, *remote_port;
	int rv, numbytes;
	int i, command, msgExists, ready;

	char buf[400];	/* Space for DH prime as well as connection information. */

	char msgRecv[MAXMESSAGELENGTH], msgSend[MAXMESSAGELENGTH];

	struct timeval tv;
	fd_set readfs;

	char dataR[MAXMESSAGELENGTH], dataL[MAXMESSAGELENGTH];
	char xorKeyR[257], xorKeyL[257];
	unsigned char *hashR = malloc(sizeof(unsigned char) * 65);	/* 512bit hash, plus null byte. */
	unsigned char *hashL = malloc(sizeof(unsigned char) * 65);
	int nHKDF = 0;


	strcpy(dataR, "");
	strcpy(dataL, "");
	
	if (argc != 2)
	{
		printf("usage: ./client [hostname].\n");
		exit(-1);
	}
	serv_hostname = argv[1];

	dh = DH_new();
	if (dh == NULL)
	{
		perror("client: DH_new");
		exit(-1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(serv_hostname, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(-1);
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
		exit(-1);
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), remoteIP, sizeof remoteIP);
	printf("Connecting to %s\n", remoteIP);

	freeaddrinfo(servinfo);

	/* Receive Diffie-Hellman prime. */
	if ((numbytes = recv(server_fd, buf, sizeof(buf), 0)) == -1)
	{
		perror("recv");
		exit(-1);
	}
	buf[numbytes] = '\0';

	BN_hex2bn(&dh->p, strtok(buf, ":"));
	BN_dec2bn(&dh->g, "2");

	command = atoi(strtok(NULL, ":"));

	while(1)
	{
		/* Receive - 0:(wait)
					 1:[local port]:[hostname]:[remote port]
					 2:[message flag]:[message]
					 3:[message flag]:[message]
		*/

		switch (command)
		{
		case 0:	/* Wait. */
			ready = 0;
			printf("Waiting for Bruce Schneier...\n");
			break;
		case 1:	/* Get hostname and ports. */
			ready = 1;
			local_port = strtok(NULL, ":");
			client_hostname = strtok(NULL, ":");
			remote_port = strtok(NULL, "\0");
			client_fd = ConnectToClient(local_port, client_hostname, remote_port);
			
			printf(">>");
			fflush(stdout);
			break;
		case 2:	/* Receive message. */
			ready = 1;
			msgExists = atoi(strtok(NULL, ":"));

			if (msgExists == 1)
			{
				strcpy(msgRecv, strtok(NULL, "\0"));

				printf("RECEIVED: '%s'\n", msgRecv);
				printf(">>");
				fflush(stdout);
			}
			break;
		case 3: /* Receive message, then wait for new connections info. */
			ready = 0;
			msgExists = atoi(strtok(NULL, ":"));

			if (msgExists == 1)
			{
				strcpy(msgRecv, strtok(NULL, "\0"));

				printf("RECEIVED: '%s'\n", msgRecv);
				printf(">>");
				fflush(stdout);
			}
			break;
		default:
			fprintf(stderr, "Something weird happened...\n");
			break;
		}

		if (ready == 1)
		{
			strcpy(msgSend, "");

			tv.tv_sec = 0;
			tv.tv_usec = 500000;

			FD_ZERO(&readfs);
			FD_SET(0, &readfs);

			if (select(1, &readfs, NULL, NULL, &tv) == -1)
			{
				perror("select");
				exit(-1);
			}

			/* Read message from stdin. */
			if (FD_ISSET(0, &readfs))
			{
				fgets(msgSend, sizeof(msgSend), stdin);
				msgSend[strlen(msgSend) - 1] = 0;	/* Get rid of newline. */
				printf("[DEBUG] Sent '%s'\n", msgSend);
			}

			/* Get keys to xor with message. */
			for (i = 0; i < 4; i++)
			{
				nHKDF++;
				memcpy(hashR, HMAC(EVP_sha512(), SKRight, 128, (unsigned char*)dataR, 
					strlen(dataR), NULL, NULL), 65);
				memcpy(hashL, HMAC(EVP_sha512(), SKLeft, 128, (unsigned char*)dataL, 
					strlen(dataL), NULL, NULL), 65);
				sprintf(&xorKeyR[i*64], "%s", hashR);
				sprintf(&xorKeyL[i*64], "%s", hashL);

				sprintf(dataR, "%s%c%c%c%c", hashR, (nHKDF >> 24) & 0xFF, (nHKDF >> 16) & 0xFF,
					(nHKDF >> 8) & 0xFF, nHKDF & 0xFF);
				sprintf(dataL, "%s%c%c%c%c", hashL, (nHKDF >> 24) & 0xFF, (nHKDF >> 16) & 0xFF,
					(nHKDF >> 8) & 0xFF, nHKDF & 0xFF);
			}

			/* xor message with keys. */
			for (i = 0; i < strlen(msgSend); i++)
			{
				msgSend[i] ^= (xorKeyR[i] ^ xorKeyL[i]);
			}
			for (; i < MAXMESSAGELENGTH; i++)
			{
				msgSend[i] = xorKeyR[i] ^ xorKeyL[i];
			}

			/* Send xor'd message to server. */
			if (send(server_fd, msgSend, MAXMESSAGELENGTH-1, 0) == -1)
			{
				perror("send");
				exit(-1);
			}
		}

		if ((numbytes = recv(server_fd, buf, MAXMESSAGELENGTH-1, 0)) == -1)
		{
			perror("recv");
			exit(-1);
		}
		buf[numbytes] = '\0';

		command = atoi(strtok(buf, ":"));
	}

	return 0;
}














