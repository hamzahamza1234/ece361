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
#include <ctype.h>

#define _OPEN_SYS_SOCK_IPV6
#define MAXDATASIZE 100

int main(int argc, char *argv[])
{
    int sockfd;          // to hold the socket file descriptor
    socklen_t addr_size; // holds the size of the servers IP address
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    char s[INET_ADDRSTRLEN];

    // variales needed for login
    char client_name[100];
    char client_pass[100];
    char server_address[100];
    char server_p[100];
    int server_port;


    while(1){
        char command[4096];
        char login_com[4096];

        printf("New Client has started, Please enter a command\n");
        fgets(command, MAXDATASIZE, stdin);

        //check for /quit or /login first because only those commands can be done first
        char login_check[4096];
        strncpy(login_check, command, 6);

        while (strcmp(login_check, "/login") != 0 && strcmp(command, "/quit\n") != 0)
        {
            printf("Please Try Again, Enter the login or quit command with correct Arguments\n");
            fgets(command, MAXDATASIZE, stdin);
            strncpy(login_check, command, 6);
        }
        

        if (strcmp(command,"/quit\n") == 0){
            break;
        }
        else {
            char delim[]=" ";
            int num_args = 0;
            strncpy(login_com, command+7,MAXDATASIZE );

            char* login_ptr = strtok(login_com,delim);

            while(login_ptr != NULL){
                if (num_args == 0){
                   strncpy(client_name, login_ptr,100);
                }
                else if (num_args == 1)
                {
                    strncpy(client_pass, login_ptr, 100);
                }
                else if (num_args == 2)
                {
                    strncpy(server_address, login_ptr, 100);
                }
                else if (num_args == 3)
                {
                    strncpy(server_p, login_ptr, 100);
                }

                num_args++;
                login_ptr = strtok(NULL, delim);
            }

            if (num_args != 4){
                printf("Wrong number of Arguments\n");
                continue;
            }

        }
        int num;
        char ch;

        if ((sscanf(server_p, "%i%c", &num, &ch) == 0))
        {
            printf("port number has to be a NUMBER\n");
            continue;
        }

       server_port = atoi(server_p);


       printf("client name: %s\n", client_name);
       printf("client pass: %s\n", client_pass);
       printf("client ip: %s\n", server_address);
       printf("server port: %d\n", server_port);

       return 0;

        
      

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

    while(1){

    char str[MAXDATASIZE];

    // this is to get the filename from the terminal
    printf("Enter the message (or enter exit): \n");
    fgets(str,MAXDATASIZE, stdin );
    int len = strlen(str);

    char exit_msg[100]= "exit\n";

    if (strcmp(str,exit_msg) == 0){
        break;
    }

    if (send(sockfd , str , len, 0) == -1){
        perror("send");
        close(sockfd);
        exit(0);
    }

    }

    printf("closing connection \n");

    freeaddrinfo(servinfo);
    close(sockfd);
    }

}
