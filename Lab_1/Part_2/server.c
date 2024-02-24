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
#include <assert.h>

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
    struct addrinfo hints, *res;
    int sockfd, new_fd;
    struct sockaddr_storage from_addr;
    socklen_t addr_size;
    time_t t;

    // Intialize random number generator
    srand((unsigned) time(&t));

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
    int next_packet = 1;
    FILE* new_file_ptr;
    while(1) {
        char buffer[4096] = {'\0'}; //to hold the message reciever

        /// UDP is connectionless so no listening or accepting. just recvfrom to listen and sento to send
        int length = sizeof(struct sockaddr_storage);
        int num_bytes = recvfrom(sockfd, (char *)buffer, 4096, 0, (struct sockaddr *) &from_addr, &length); //here from addr will store the address of client

        if (num_bytes < 0) {
            perror("failed to get message , 0 bytes recieved ");
            exit(EXIT_FAILURE);
        }

        if (waiting_for_ftp) {
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
                next_packet = 1;
            }
            else {
                printf("Sending no to client\n");
                if ((sent_bytes = sendto(sockfd, reply_no, strlen(reply_no), 0, (const struct sockaddr *)&from_addr, length)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }
            }
        } else {
            // Randomly determine if a packet is dropped
            // A higher drop_frequency mean that the chance of dropping is lower (packets drop 1 in drop_frequncy times)
            int drop_frequency = 100;
            //if (rand() % drop_frequency > 0 ) {
                // Packet is accepted
                struct packet file_packet;
                char name[200];

                // Converting the message received from the client into the struct packet
                sscanf(buffer, "%u:%u:%u:", &file_packet.total_frag, &file_packet.frag_no, &file_packet.size);
                
                int name_start_index = 0, name_end_index = 0, num_colon = 0;
                for (int i = 0; i < num_bytes; i++) {
                    if (buffer[i] == ':') {
                        num_colon++;
                        if (num_colon == 3) {
                            name_start_index = i + 1;
                        }
                        if (num_colon == 4) {
                            name_end_index = i;
                        }
                    }
                }
                memcpy(name, &buffer[name_start_index], name_end_index - name_start_index);
                file_packet.filename = name;
                memcpy(file_packet.filedata, &buffer[name_end_index + 1], file_packet.size);
                //int num = rand();
                //printf("rand() = %d\n", num);

                if (rand() % drop_frequency == 0 ) {
                    printf("Packet %d dropped\n", file_packet.frag_no);
                    // Send NACK
                    char reply_no[100] = "NACK";
                    int sent_bytes;
                    if ((sent_bytes = sendto(sockfd, reply_no, strlen(reply_no), 0, (const struct sockaddr *)&from_addr, length)) == -1) {
                        perror("talker: sendto");
                        exit(1);
                    }
                } else {

                // Need to check to make sure that packets are being processed in expected order
                //if (file_packet.frag_no != next_packet) {
                //    printf("file_packet.frag_no = %d, next_packet = %d\n", file_packet.frag_no, next_packet);
                //}
                assert (file_packet.frag_no == next_packet);
                ///if (file_packet.frag_no == next_packet) {
                    next_packet++;

                    // Open the file stream if this is the first packet
                    if (file_packet.frag_no == 1) {
                        new_file_ptr = fopen(file_packet.filename, "w");
                    }

                    // Write filedata to the filestream
                    fwrite(file_packet.filedata, 1, file_packet.size, new_file_ptr);

                    // Close the file stream if this is the last packet
                    if (file_packet.frag_no == file_packet.total_frag) {
                        fclose(new_file_ptr);
                        printf("File transfer complete\n");

                        // This file transfer is complete so wait for a new ftp message
                        waiting_for_ftp = true;
                    }
                //}

                // Send packet acknowledgement
                char reply_yes[100] = "ACK";
                int sent_bytes;
                //printf("Packet %d Acknowledgement\n", file_packet.frag_no);
                if ((sent_bytes = sendto(sockfd, reply_yes, strlen(reply_yes), 0, (const struct sockaddr *)&from_addr, length)) == -1) {
                    perror("talker: sendto");
                    exit(1);
                }
                }
            /*} else {
                // Packet is dropped
                printf("Packet dropped\n");
            }*/
        }
    }

    //close the socket in the end

    //close(sockfd);

    return 0;
}