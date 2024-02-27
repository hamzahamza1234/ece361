#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#define BACKLOG 20
#define MAXDATASIZE 100


int main(int argc, char *argv[])
{
    struct addrinfo hints, *res;
    int sockfd, new_fd ,num_bytes;
    struct sockaddr_storage from_addr;
    socklen_t addr_size;
    char s[INET_ADDRSTRLEN];
    char buf[MAXDATASIZE];

    addr_size = sizeof(from_addr);

    char buffer[4096]; // to hold the message reciever

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM; // use UDP sockets
    hints.ai_flags = AI_PASSIVE;    // fill in my IP for me

    getaddrinfo(NULL, argv[1], &hints, &res); // specified port number in terminal

    // make a socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0)
    {
        perror("failed to create socket");
        exit(EXIT_FAILURE);
    }

    // bind it to the port we passed in to getaddrinfo():
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("failed to bind");
        exit(EXIT_FAILURE);
    }


    while(1){

        if (listen(sockfd, BACKLOG) == -1){
            perror("listen failed");
            exit(1);
        }

        new_fd = accept(sockfd, (struct sockaddr*) &from_addr, &addr_size);

        if (new_fd == -1){
            perror("accept failed");
            continue;
        }

        if ( *inet_ntop(from_addr.ss_family, (struct sockaddr *)&from_addr, s, sizeof (s)) == -1 ){
            perror("inet_ntop failed");
        }
        printf("server: got connection from %s\n", s);

        if ((num_bytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == -1)
        {
            perror("recv failed");
            exit(1);
        }
        buf[num_bytes] = '\0';
        printf("SERVER : received '%s'\n", buf);
    }


    return 0;
}