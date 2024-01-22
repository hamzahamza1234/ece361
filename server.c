#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BACKLOG 10

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res;
    int sockfd,new_fd;
    struct sockaddr_storage from_addr;
    socklen_t addr_size;

    char buffer[4096]; //to hold the message reciever

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_DGRAM;  //use UDP sockets
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    getaddrinfo(NULL, argv[1], &hints, &res); //specified port number in terminal
    // make a socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0){
        perror("failed to create socket");
        exit(EXIT_FAILURE);
    }
    //bind it to the port we passed in to getaddrinfo():
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0)
    {
        perror("failed to bind");
        exit(EXIT_FAILURE);
    }


    /// UDP is connectionless so no listening or accepting. just recvfrom to listen and sento to send
    int length = sizeof(struct sockaddr_storage);
    int num_bytes = recvfrom(sockfd, (char *)buffer, 4096, 0, (struct sockaddr *) &from_addr, &length);

    if (num_bytes < 0)
    {
        perror("failed to get message , 0 bytes recieved ");
        exit(EXIT_FAILURE);
    }

    buffer[length] = '\0';

    printf("msg recieved: %s \n", buffer);

    close(sockfd);

    return 0;


}