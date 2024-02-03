#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#define BACKLOG 10

struct packet
{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char *filename;
    char filedata[1000];
};

int main(int argc, char *argv[]) {
    /*// Testing string merging
    char Yes[100] = "yes ";
    int i = 10;
    char Num_buff[100];
    sprintf(Num_buff, "%d", i);
    strcat(Yes, Num_buff);
    printf("%s \n", Yes);
*/
    struct addrinfo hints, *res;
    int sockfd, new_fd;
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
    if (sockfd < 0) {
        perror("failed to create socket");
        exit(EXIT_FAILURE);
    }


    //bind it to the port we passed in to getaddrinfo():
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("failed to bind");
        exit(EXIT_FAILURE);
    }

    bool waiting_for_ftp = true;
    while(1) {
        //printf("%s", waiting_for_ftp ? "true\n" : "false\n");
        if (waiting_for_ftp) {
            /// UDP is connectionless so no listening or accepting. just recvfrom to listen and sento to send
            int length = sizeof(struct sockaddr_storage);
            int num_bytes = recvfrom(sockfd, (char *)buffer, 4096, 0, (struct sockaddr *) &from_addr, &length); //here from addr will store the address of client

            if (num_bytes < 0) {
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

            if (strcmp(check, buffer) == 0) {
                printf("Sending yes to client\n");
                if ((sent_bytes = sendto(sockfd, reply_yes, strlen(reply_yes), 0, (const struct sockaddr *)&from_addr, length)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }
                waiting_for_ftp = false;
            }
            else {
                printf("Sending no to client\n");
                if ((sent_bytes = sendto(sockfd, reply_no, strlen(reply_no), 0, (const struct sockaddr *)&from_addr, length)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }
            }
        } else {
            //printf("Waiting for file packets\n");
            waiting_for_ftp = true;
        }
    }


    //close the socket in the end

    close(sockfd);

    return 0;


}