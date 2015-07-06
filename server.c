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
#define MAXDATASIZE 500

typedef struct client
{
    int fd;
    char hostname[INET6_ADDRSTRLEN];
    char lport[6];
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

    char buf[MAXDATASIZE];
    char msg[MAXDATASIZE];
    char xor[MAXDATASIZE];
    int numbytes;
    LIST* msgList;
    LIST* clientList;
    char* port = "30002";
    char temp[6];

    //need if using fork()
    //struct sigaction sa;  
    int yes=1;
    char remoteIP[INET6_ADDRSTRLEN];
    int i, j, rv;

    int numClients = 0;
    int msgCount = 0;
    

    clientList = ListCreate();
    msgList = ListCreate();

    DH *dh;
    int prime_len;
    int generator = 2;

    //clear file descriptor sets
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    if (argc == 2)
        prime_len = atoi(argv[1]);
    else
        prime_len = 1024;

    dh = DH_new();
    DH_generate_parameters_ex(dh, prime_len, generator, NULL);
    printf("  p = %s\n", BN_bn2hex(dh->p));

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

    if (p == NULL) 
    {
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
                    /* New connection */
                    sin_size = sizeof their_addr;

                    new_fd = accept(listener, (struct sockaddr *)&their_addr, &sin_size);
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

                        ListLast(clientList);   //TODO: Find the right place to add client
                        ListAdd(clientList, malloc(sizeof(CLIENT)));
                        ((CLIENT *)clientList->cur->item)->fd = new_fd;
                        strcpy(((CLIENT *)clientList->cur->item)->hostname, inet_ntop(their_addr.ss_family, 
                            get_in_addr((struct sockaddr*)&their_addr), remoteIP, INET6_ADDRSTRLEN));
                        strcpy(((CLIENT *)clientList->cur->item)->lport, port);
                        //port = itoa(atoi(port) + 1);
                        sprintf(temp, "%d", atoi(port) + 1);
                        port = temp;
                        numClients++;

                        printf("%d: %d: %s\n", numClients, ((CLIENT *)clientList->cur->item)->fd, ((CLIENT *)clientList->cur->item)->hostname);

                        //strcpy(msg, BN_bn2hex(dh->p));
                        //strcat(msg, ":");
                        //if (send(new_fd, msg, strlen(msg), 0) == -1)
                        //{
                        //    perror("send");
                        //}

                        if (numClients < 3)
                        {
                            /* Not enough clients 
                               Tell them to wait. */
                            strcpy(msg, BN_bn2hex(dh->p));
                            strcat(msg, ":");
                            strcat(msg, "0:");
                            if (send(new_fd, msg, strlen(msg), 0) == -1)
                            {
                                perror("send");
                            }
                        }
                        else if (numClients == 3)
                        {
                            /* Third client arrived.
                               Tell everyone to start. */

                            ListFirst(clientList);

                            for (j = 0; j <= fdmax; j++)
                            {
                                if (FD_ISSET(j, &master))
                                {
                                    // Don't send to listener
                                    if (j != listener)
                                    {
                                        //if(ListLast(clientList))
                                        //    ListFirst(clientList);
                                        //else
                                        //    ListNext(clientList);

                                        /* 1:[sender port]:[hostname]:[receiver port] */
                                        //strcpy(msg, "");
                                        if(j == new_fd)     //Send prime
                                        {
                                            strcpy(msg, BN_bn2hex(dh->p));
                                            strcat(msg, ":");
                                            strcat(msg, "1:");
                                        }
                                        else
                                        {
                                            strcpy(msg, "1:");
                                        }
                                        //printf(" _ %s\n", ((CLIENT *)clientList->cur->item)->lport);
                                        strcat(msg, ((CLIENT *)clientList->cur->item)->lport);
                                        //printf(" _ %s\n", ((CLIENT *)clientList->cur->item)->lport);
                                        strcat(msg, ":");
                                        //printf(" _ %s\n", ((CLIENT *)clientList->cur->item)->lport);

                                        if(ListNext(clientList) == NULL)
                                            ListFirst(clientList);

                                        strcat(msg, ((CLIENT *)clientList->cur->item)->hostname);
                                        //printf(" _ %s\n", ((CLIENT *)clientList->cur->item)->lport);
                                        //printf("...%s\n", ((CLIENT *)clientList->cur->item)->lport);
                                        strcat(msg, ":");
                                        //printf(" _ %s\n", ((CLIENT *)clientList->cur->item)->lport);
                                        //printf("..\n");
                                        strcat(msg, ((CLIENT *)clientList->cur->item)->lport);  

                                        //printf(".\n");
                                        //printf(" - %s - (%lu)\n", msg, strlen(msg));
                                        if (send(j, msg, strlen(msg), 0) == -1)
                                        {
                                            perror("send");
                                        }
                                    }
                                }
                            }
                        }
                        else
                        {
                            /* Chat started, someone else joined.
                               Do something.  */
                            //Tell newest client to start, and update graph
                            if (send(i, "1", 1, 0) == -1)
                            {
                                perror("send");
                            }

                            //TODO: Lots of stuff.

                        }

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
                            numClients--;
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
                        msgCount++;
                        buf[numbytes] = '\0';
                        if(ListAdd(msgList, malloc(MAXDATASIZE)) == -1)
                            perror("ListAdd");
                        strcpy((char *)(msgList->cur->item), buf);
                        printf("Message received (%i/%i)\n", msgCount, numClients);
                        printf("    - %s\n", buf);

                        if(msgCount >= numClients)
                        {
                            //xor messages
                            strcpy(xor, ListFirst(msgList));
                            free(ListRemove(msgList));

                            while(ListCount(msgList) > 0)
                            {
                                strcpy(buf, (char *)msgList->cur->item);
                                for(j = 0; j <= MAXDATASIZE; j++)
                                {
                                    xor[j] ^= buf[j];
                                }
                                free(ListRemove(msgList));
                            }
                            strcpy(msg, "2:");
                            strcat(msg, xor);

                            /* Send message. */
                            for (j = 0; j <= fdmax; j++)
                            {
                                if (FD_ISSET(j, &master))
                                {
                                    // Don't send to listener
                                    if (j != listener)
                                    {
                                        if (send(j, msg, strlen(msg), 0) == -1)
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
