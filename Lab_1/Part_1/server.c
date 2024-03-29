#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

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

while(1){
    /// UDP is connectionless so no listening or accepting. just recvfrom to listen and sento to send
    socklen_t length = sizeof(struct sockaddr_storage);
    int num_bytes = recvfrom(sockfd, (char *)buffer, 4096, 0, (struct sockaddr *) &from_addr, &length); //here from addr will store the address of client

    if (num_bytes < 0)
    {
        perror("failed to get message , 0 bytes recieved ");
        exit(EXIT_FAILURE);
    }

    buffer[length] = '\0';

    //our reply if based on what was sent from the client , if ftp was sent , we send a yes, else we send a no

    char reply_yes[100] = "yes";
    char reply_no[100] = "no";
    int sent_bytes;

    printf("Message from Client: %s\n" , buffer);

    char check[4096] = "ftp";

    if (strcmp(check, buffer) == 0){
         printf("Sending yes to client\n");
        if ((sent_bytes = sendto(sockfd, reply_yes, strlen(reply_yes), 0, (const struct sockaddr *)&from_addr, length)) == -1)
        {
            perror("talker: sendto");
            exit(1);
        }
    }
    else{
        printf("Sending no to client\n");
        if ((sent_bytes = sendto(sockfd, reply_no, strlen(reply_no), 0, (const struct sockaddr *)&from_addr, length)) == -1)
        {
            perror("talker: sendto");
            exit(1);
        }
         
    }
}


    //close the socket in the end

   // close(sockfd);

    return 0;


}