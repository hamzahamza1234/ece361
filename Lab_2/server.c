#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>

#define BACKLOG 20
#define MAXDATASIZE 100
#define MAX_NAME 100

struct message
{
    unsigned int type;
    unsigned int size;
    char source[MAX_NAME];
    char data[MAXDATASIZE];
};

// Defining the LO_ACK message 
struct message lo_ack;
struct message lo_nack;

int main(int argc, char *argv[])
{
    struct addrinfo hints, *res;
    int sockfd, new_fd ;
    struct sockaddr_storage from_addr;
    socklen_t addr_size;
    char s[INET_ADDRSTRLEN];
    char buf[MAXDATASIZE];
    char zero[100]= {'0' };

    addr_size = sizeof(from_addr);

    char buffer[4096]; // to hold the message reciever

    //defining the lo ack message so early because it doesnt change

    lo_ack.type = 2; //index based on the table in the document
    lo_ack.size = 0;
    strncat(lo_ack.source, zero, 2);
    strncat(lo_ack.data,zero , 2);

    char lo_ack_buffer[4096] = {'\0'};
    char num_buffer[4096] = {'\0'};
    char colon_str[2] = ":";

    sprintf(num_buffer, "%d", lo_ack.type);
    strcat(lo_ack_buffer, num_buffer);
    strcat(lo_ack_buffer, colon_str);

    sprintf(num_buffer, "%d", lo_ack.size);
    strcat(lo_ack_buffer, num_buffer);
    strcat(lo_ack_buffer, colon_str);

    strcat(lo_ack_buffer, lo_ack.source);
    strcat(lo_ack_buffer, colon_str);

    strcat(lo_ack_buffer, lo_ack.data);

    int len_lo_ack_msg = strlen(lo_ack_buffer);

    //defining the lo nack message as well

    lo_nack.type = 3; // index based on the table in the document
    lo_nack.size = 0;
    strncat(lo_nack.source, zero, 2);
    strncat(lo_nack.data, zero, 2);

    char lo_nack_buffer[4096] = {'\0'};

    sprintf(num_buffer, "%d", lo_nack.type);
    strcat(lo_nack_buffer, num_buffer);
    strcat(lo_nack_buffer, colon_str);

    sprintf(num_buffer, "%d", lo_nack.size);
    strcat(lo_nack_buffer, num_buffer);
    strcat(lo_nack_buffer, colon_str);

    strcat(lo_nack_buffer, lo_nack.source);
    strcat(lo_nack_buffer, colon_str);

    strcat(lo_nack_buffer, lo_nack.data);

    int len_lo_nack_msg = strlen(lo_nack_buffer);


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM; // use TCP sockets
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

        printf("looking for a new connextion\n");

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
 
        int num_bytes = 0;

        if ((num_bytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == 0) // this makes sure that if client closes its fd, we stop listening and close ours as well
        {
            close(new_fd);
            printf("closing connection\n");
            perror("recv fininshed");
            continue;
        }
        buf[num_bytes] = '\0';
        printf("SERVER : received : %s \n", buf);

        if (1){ // need to check login message for authentication here (i simply did 1 for now)
            if (send(new_fd, lo_ack_buffer, len_lo_ack_msg, 0) == -1)
            {
                perror("send");
                close(sockfd);
                exit(0);
            }

            printf("Client has been authenicated and joined connection\n");
        }
        else{  //if authentication failed send a lo_nack message and continue searching for clients
            if (send(new_fd, lo_nack_buffer, len_lo_nack_msg, 0) == -1)
            {
                perror("send");
                close(sockfd);
                exit(0);
            }
            printf("Client Authenication Failed\n");
            continue;

        }

        // this loop makes sure we keep listening to our client until it closes and echoing the message back
        // I have added this echo to make sure my client is able to both read from server and stdin at the same time

        while(num_bytes != 0){  
            num_bytes = 0;
            if ((num_bytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == 0 )
            {
                close(new_fd);
                printf("Client has closed connection\n");
                perror("recv finished");
                break;
            }
            buf[num_bytes] = '\0';
            printf("SERVER : received : %s \n", buf);

            printf("Sending '%s' Back to Client. \n" , buf);

            if (send(new_fd, buf , num_bytes, 0) == -1)
            {
                perror("send");
                close(sockfd);
                exit(0);
            }
        }


        
    }


    return 0;
}