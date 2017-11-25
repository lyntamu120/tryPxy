/*
** server.c -- a stream socket server demo
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>


#define PORT "5000" // the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold
#define MAXNUMOFCACHE 10 // how many files will be cached

#include "structures.h"

struct Pages cache[MAXNUMOFCACHE];
int numOfFile = 0;

int main(void) {
    int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p, *res;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    void *addr;
    char buf[2056], doc[512], host[512];
    int byte_count;


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        addr = &(ipv4->sin_addr);
        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, s, sizeof s);
        printf("The sockfd from server is: %d\n", sockfd);
        break;
    }
    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    printf("server: waiting for connections on %s...\n", s);

    while(1) { // main accept() loop
        printf("At the beginning of the while, current size of the cache is: %d\n", numOfFile);
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        // printf("The newsockfd from server is: %d\n", new_fd);

        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server: got connection from %s\n", s);
        printf("current size of the cache is: %d\n", numOfFile);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            if (recv(new_fd, buf, sizeof buf, 0) == -1) {
                perror("send");
            }
            printf("%s\n", buf);

            parseHostAndDoc(&buf[0], &doc[0], &host[0]);
            printf("%s\n", doc);
            printf("%s\n", host);
            int docInCache = findInCache(&buf[0]);
            if (docInCache == -1) {
                //cache doesn't contain the doc
                cacheHTTPRequest(&buf[0], &host[0], &doc[0]);
            } else {
                printf("%s\n", "Containing the file!");
                //cache contains the doc
                update(docInCache);
            }
            sendFileToClient(new_fd, &doc[0]);
            close(new_fd);
            exit(0);
        }
        close(new_fd); // parent doesn't need this
    }
    return 0;
}
