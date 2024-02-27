#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#define _OPEN_SYS_SOCK_IPV6



int main(int argc, char *argv[])
{
    int sockfd;          // to hold the socket file descriptor
    socklen_t addr_size; // holds the size of the servers IP address
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    char s[INET_ADDRSTRLEN];

    if (argc != 3)
    { // just to check if both the IP address and port number were given as arguments
        fprintf(stderr, "usage: talker hostname message\n");
        exit(1);
    }

    memset(&hints, 0, sizeof(hints));
    // filling up some information about the server

    hints.ai_family = AF_INET;      // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_STREAM; // for a UDP connection

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
    { // here we get the infomration based on the IP address and port number given
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("Socket Failed");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client: connect");
            continue;
        }
            break;
        }

    // in case we couldnt make a socket out of the IP address and port number given
    if (p == NULL)
    {
        fprintf(stderr, "Failed to create socket and connect\n");
        return 2;
    }

     if ( *inet_ntop(p->ai_family , (struct sockaddr *)p->ai_addr, s, sizeof (s)) == -1 ){
            perror("inet_ntop failed");
        }

    printf("client: connecting to %s\n", s);

    char str[200];

    // this is to get the filename from the terminal
    printf("Enter the message: \n");
    scanf("%[^\n]s \n", str);
    int len = strlen(str);

    if (send(sockfd , str , len, 0) == -1){
        perror("send");
        close(sockfd);
        exit(0);
    }


    freeaddrinfo(servinfo);
    close(sockfd);

}
