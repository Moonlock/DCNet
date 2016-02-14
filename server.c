#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "list/list.h"

#include <openssl/dh.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#define PORT "30001"
#define BACKLOG 10
#define MAXMESSAGELENGTH 257		/* Messages are 256 bytes, plus nul byte. */


typedef struct client
{
	int fd;
	char hostname[INET6_ADDRSTRLEN];
	char port[6];
} CLIENT;


DH *dh;


/* Get sockaddr, IPv4 or IPv6. */
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Comparison function for ListSearch(). */
int CompareClients(void* a, void* b)
{
	return ((CLIENT *)a)->fd == *(int *)b;
}

/* Error-checked malloc. */
void *ec_malloc(unsigned int size)
{
	void *ptr;
	ptr = malloc(size);

	if (ptr == NULL)
	{
		perror("server: malloc");
		exit(-1);
	}

	return ptr;
}

/* If new_fd is nonzero, it's the file descriptor of the client that has not received the DH prime yet. */
void UpdateConnections(LIST* clientList, int fdmax, fd_set master, int listener, int new_fd)
{
	CLIENT* client;
	char msg[400];	/* Space for DH prime as well as connection information. */
	int j;

	client = ListFirst(clientList);

	for (j = 0; j <= fdmax; j++)
	{
		if (FD_ISSET(j, &master) && j != listener)
		{
            /* [Port to listen on]:[host to send to]:[port to send to] */
			if ((new_fd != 0) && (j == new_fd))
			{
                /* Newest client didn't recieve prime yet. */
				strcpy(msg, BN_bn2hex(dh->p));
				strcat(msg, ":1:");
			}
			else
			{
				strcpy(msg, "1:");
			}

			strcat(msg, client->port);
			strcat(msg, ":");

			client = ListNext(clientList);
			if(client == NULL)
				client = ListFirst(clientList);

			strcat(msg, client->hostname);
			strcat(msg, ":");
			strcat(msg, client->port);

			if (send(j, msg, strlen(msg), 0) == -1)
			{
				perror("send");
			}
		}
	}
}

int main(int argc, char const *argv[])
{
	fd_set master;
	fd_set read_fds;	/* Temp file descriptor list. */
	int fdmax;

	int listener, new_fd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;

	char client_port[6];
	char buf[MAXMESSAGELENGTH];
	char msg[MAXMESSAGELENGTH + 4];	/* 4 Bytes of header before message. */
	char xor[MAXMESSAGELENGTH];
	char *message;
	int numbytes;

	CLIENT *client;
	LIST *msgList, *clientList;
	int msgCount = 0;
	int numClients = 0;
	int newClients = 0;

	char remoteIP[INET6_ADDRSTRLEN];
	int yes=1;
	int i, j, rv;

	int prime_len = 1024;	/* Default value. */
	int generator = 2;


	if (argc > 1)
		prime_len = atoi(argv[1]);

	dh = DH_new();
	DH_generate_parameters_ex(dh, prime_len, generator, NULL);
	printf("[DEBUG] DH prime: %s\n", BN_bn2hex(dh->p));

	clientList = ListCreate();
	msgList = ListCreate();

	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;



	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(-1);
	}

	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
		{
			perror("server: socket");
			continue;
		}

		if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
		{
			perror("server: setsockopt");
			exit(-1);
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
		exit(-1);
	}

	freeaddrinfo(servinfo);

	if (listen(listener, BACKLOG) == -1)
	{
		perror("listen");
		exit(-1);
	}

	FD_SET(listener, &master);
    fdmax = listener;	/* Only file descriptor so far. */



	while(1)
	{
		read_fds = master;

		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
		{
			perror("select");
			exit(-1);
		}

    	/* Run through connections, looking for data to read. */
		for(i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &read_fds))
			{
				if (i == listener)
				{
    				/* New client. */

					sin_size = sizeof their_addr;

					new_fd = accept(listener, (struct sockaddr *)&their_addr, &sin_size);
					if (new_fd == -1)
					{
						perror("accept");
					}
					else
					{
                    	/* Add to master, keep track of biggest file descriptor */
						FD_SET(new_fd, &master);
						if (new_fd > fdmax)
						{
							fdmax = new_fd;
						}

                    	/* Keep a list of clients' port, fd, and hostname. */
						client = (CLIENT*) ec_malloc(sizeof(CLIENT));
						if (new_fd < 10)
							sprintf(client_port, "3000%i", new_fd);
						else
							sprintf(client_port, "300%i", new_fd);

						strcpy(client->hostname, inet_ntop(their_addr.ss_family, 
							get_in_addr((struct sockaddr*)&their_addr), remoteIP, INET6_ADDRSTRLEN));
						strcpy(client->port, client_port);
						client->fd = new_fd;

						ListAppend(clientList, client);                    
						
						printf("[DEBUG] numClients:%d, fd:%d, hostname:%s\n", 
							numClients, client->fd, client->hostname);

						if (numClients < 2)
						{
                    		/* Not enough clients.
                    		   Send them DH prime and tell them to wait. */
							numClients++;

							strcpy(msg, BN_bn2hex(dh->p));
							strcat(msg, ":0:");
							if (send(new_fd, msg, strlen(msg), 0) == -1)
							{
								perror("send");
							}
						}
						else if (numClients == 2)
						{
                    		/* Third client just arrived.
                    		   Tell everyone to update connections. */
							numClients++;

                    		UpdateConnections(clientList, fdmax, master, listener, new_fd);								
						}
						else
						{
							/* Chat has started and another client joined.
							   New client will be added in the next round. */
							
							newClients++;	

							strcpy(msg, BN_bn2hex(dh->p));
							strcat(msg, ":0:");
							if (send(new_fd, msg, strlen(msg), 0) == -1)
							{
								perror("send");
							}
						}
						printf("server: new connection from %s on socket %d\n",
							inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr),
								remoteIP, INET6_ADDRSTRLEN), new_fd);
					}
				}
				else
				{
    				/* Receive data from client. */
					if ((numbytes = recv(i, buf, (sizeof buf), 0)) <= 0)
					{
						if (numbytes == 0)
						{
    						/* Connection closed. */
							printf("server: Socket %d hung up.\n", i);
						}
						else
						{
							perror("recv");
						}

						close(i);
						FD_CLR(i, &master);
						numClients--;

    					/* Take client out of list. */
						if (ListSearch(clientList, CompareClients, &i) == NULL)
						{
							fprintf(stderr, "ListSearch: Failed to find client with fd %d.\n", i);
						}
						else
						{
							free(ListRemove(clientList));
						}
                    	
                    	UpdateConnections(clientList, fdmax, master, listener, 0);													
					}
					else
					{
						/* Data from client. */

						msgCount++;
						buf[numbytes] = '\0';

						message = (char *)ec_malloc(MAXMESSAGELENGTH);
						strcpy(message, buf);
						if(ListAdd(msgList, message) == -1)
							perror("ListAdd");

						if (msgCount >= numClients)	/* Received a message from each client. */
						{
							/* XOR messages */
							strcpy(xor, ListFirst(msgList));
							free(ListRemove(msgList));

							while(ListCount(msgList) > 0)
							{
								message = (char *)msgList->cur->item;
									//TODO: write ListCur() function.
								for(j = 0; j < MAXMESSAGELENGTH; j++)
								{
									xor[j] ^= message[j];
								}
								free(ListRemove(msgList));
							}

							if (newClients == 0)
							{
								if (strcmp(xor, "") == 0)
								{
									/* No one sent a message. */
									strcpy(msg, "2:0:");
								}
								else
								{
									/* Prepare to send xor'd message. */
									strcpy(msg, "2:1:");
									strcat(msg, xor);
								}

								/* Send message to all clients. */
								for (j = 0; j <= fdmax; j++)
								{
									if (FD_ISSET(j, &master) && j != listener)
									{
										if (send(j, msg, strlen(msg), 0) == -1)
	                                    {
	                                        perror("send");
	                                    }
									}
								}
								msgCount = 0;
							}
							else	/* New client(s) waiting to join.  
									   Send message then update connections to include them. */
							{
								if (strcmp(xor, "") == 0)
								{
									/* No one sent a message. */
									strcpy(msg, "3:0:");
								}
								else
								{
									/* Prepare to send xor'd message. */
									strcpy(msg, "3:1:");
									strcat(msg, xor);
								}

								/* Send message to all clients. */
								for (j = 0; j <= fdmax; j++)
								{
									if (FD_ISSET(j, &master) && j != listener)
									{
										if (send(j, msg, strlen(msg), 0) == -1)
	                                    {
	                                        perror("send");
	                                    }
									}
								}
								numClients += newClients;
								newClients = 0;
								msgCount = 0;

								UpdateConnections(clientList, fdmax, master, listener, 0);
							}
						}
					}
				}
			}
		}
	}

return 0;
}
















