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


#define PORT "30001"
#define BACKLOG 10

typedef struct client
{
    int fd;
    char hostname[INET6_ADDRSTRLEN];
} CLIENT;

 void *get_in_addr(struct sockaddr *sa)
 {
 	if (sa->sa_family == AF_INET) 
 	{
 		return &(((struct sockaddr_in*)sa)->sin_addr);
 	}

 	return &(((struct sockaddr_in6*)sa)->sin6_addr);
 }


 int main(int argc, char *argv[])
 {
    fd_set master;
    fd_set read_fds;    //temp file descriptor list
    int fdmax;

 	int listener, new_fd; 
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; 
    socklen_t sin_size;

    char buf[256];
    int numbytes;
    LIST* clientList;
    //LIST* msgs;

    //need if using fork()
    //struct sigaction sa;  
    int yes=1;
    char remoteIP[INET6_ADDRSTRLEN];
    int i, j, rv;

    int clients = 0;
    int msgCount = 0;

    clientList = ListCreate();

    //clear file descriptor sets
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

 	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;


    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) 
    {
    	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
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
            exit(1);
        }

    	if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) 
    	{
    		close(listener);
    		perror("server: bind");
    		continue;
    	}

    	break;
    }

    if (p == NULL) {
    	fprintf(stderr, "server: failed to bind socket\n");
    	return 2;
    }

    freeaddrinfo(servinfo);

    if (listen(listener, BACKLOG) == -1) 
    {
        perror("listen");
        exit(1);
    }

    FD_SET(listener, &master);

    fdmax = listener;   //Biggest (only) file descriptor so far

    //need if using fork()
/*    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) 
    {
        perror("sigaction");
        exit(1);
    }
*/

    while(1)
    {
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(4);
        }

        // Run through connections, look for data to read.
        for(i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == listener)
                {
                    // Handle new connection
                    sin_size = sizeof their_addr;
                    new_fd = accept(listener, 
                        (struct sockaddr *)&their_addr, &sin_size);

                    if (new_fd == -1)
                    {
                        perror("accept");
                    }
                    else
                    {
                        // Add to master, keep track of max
                        FD_SET(new_fd, &master);
                        if (new_fd > fdmax)
                        {
                            fdmax = new_fd;
                        }

                        ListLast(clientList);
                        ListAdd(clientList, malloc(sizeof(CLIENT)));
                        ((CLIENT *)clientList->cur->item).fd = i;
                        strcpy(((CLIENT *)clientList->cur->item).hostname, inet_ntop(their_addr.ss_family, 
                            get_in_addr((struct sockaddr*)&their_addr), remoteIP, INET6_ADDRSTRLEN));
                        clients++;
/*
                        if (clients < 3)
                        {
                            //Not enough clients
                            if (send(i, "0", 1, 0) == -1)
                            {
                                perror("send");
                            }
                        }
                        else if (clients == 3)
                        {
                            //Send 'start' to all clients
                            for (j = 0; j <= fdmax; j++)
                            {
                                if (FD_ISSET(j, &master))
                                {
                                    // Don't send to listener
                                    if (j != listener)
                                    {
                                        if (send(j, "1", 1, 0) == -1)
                                        {
                                            perror("send");
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            //Tell newest client to start, and update graph
                            if (send(i, "1", 1, 0) == -1)
                            {
                                perror("send");
                            }


                        }
*/
                        printf("server: new connection from %s on socket %d\n",
                            inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr),
                                remoteIP, INET6_ADDRSTRLEN),
                            new_fd);
                    }
                }
                else
                {
                    // Handle data from client
                    if ((numbytes = recv(i, buf, sizeof buf, 0)) <= 0)
                    {
                        // Error or connection closed
                        if (numbytes == 0)
                        {
                            // Connection closed
                            clients--;
                            printf("server: socket %d hung up.\n", i);
                        }
                        else
                        {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master); // Remove from master set
                    }
                    else
                    {
                        // Data from client
                        //buf[numbytes] = '\0';
                        msgCount++;
                        printf("Message received (%i/%i)\n", msgCount, clients);

                        if(msgCount >= clients)
                        {
                            //printf("Sending %s\n", buf);
                            for (j = 0; j <= fdmax; j++)
                            {
                                // Send
                                if (FD_ISSET(j, &master))
                                {
                                    // Don't send to listener
                                    if (j != listener)
                                    {
                                        if (send(j, buf, numbytes, 0) == -1)
                                        {
                                            perror("send");
                                        }
                                    }
                                }
                            }
                            msgCount = 0;
                        }
                    }
                }
            }
        }
    }

    return 0;
}
