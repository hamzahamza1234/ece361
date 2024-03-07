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
#include <stdbool.h>

#define _OPEN_SYS_SOCK_IPV6
#define MAXDATASIZE 100
#define MAX_NAME 100
#define STDIN 0

bool quit = false;
bool in_session = false;

struct message
{
    unsigned int type;
    unsigned int size;
    char source[MAX_NAME];
    char data[MAXDATASIZE];
};

//defining some predefined messages
struct message query;

int main(int argc, char *argv[])
{
    int sockfd;          // to hold the socket file descriptor
    socklen_t addr_size; // holds the size of the servers IP address
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    char s[INET_ADDRSTRLEN];
    char zero[100] = {'0'};

    // variales needed for login
    char client_name[100];
    char client_pass[100];
    char server_address[100];
    char server_p[100];
    int server_port;

    fd_set read_fds; //for the select system call



    // Main While loop that the client will stay in regardless of weather its connected or not
    // Only way to exit this is my typing /quit
    while(1){

        FD_ZERO(&read_fds); // make sure its zerod out before every new connection

        if (quit == true){
            break;
        }

        char command[4096];
        char login_com[4096];
        
        struct message msg;   //this struct will store the first login_msg sent to server

        printf("New Client has started, Please enter a command\n");
        fgets(command, MAXDATASIZE, stdin);  //using fgets because scanf terminates after whitespaces

        //check for /quit or /login first because only those commands can be done first
        char login_check[4096];
        strncpy(login_check, command, 6);

        while (strcmp(login_check, "/login") != 0 && strcmp(command, "/quit\n") != 0) //if client enters anything else we will stay here
        {
            printf("Please Try Again, Enter the login or quit command with correct Arguments\n");
            fgets(command, MAXDATASIZE, stdin);
            strncpy(login_check, command, 6);
        }
        

        if (strcmp(command,"/quit\n") == 0){ // the only way to exit the program
            printf("Terminating Program\n");
            break;
        }
        else {
            msg.type = 1;  //LOGIN TYPE (indexing from 1 in the table given in the document)

            char delim[]=" ";  // used to seperate the login command to all the different arguments 
            int num_args = 0;
            strncpy(login_com, command+7,MAXDATASIZE );

            char* login_ptr = strtok(login_com,delim);

            while(login_ptr != NULL){   // this saves each argument in its respective place
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

            if (num_args != 4){  //error checking for too many arguments
                printf("Wrong number of Arguments\n");
                continue;
            }

        }
        int num;
        char ch;

        if ((sscanf(server_p, "%i%c", &num, &ch) == 0))  // error checking to make sure port is a number
        {
            printf("port number has to be a NUMBER\n");
            continue;
        }

       server_port = atoi(server_p);

       printf("client name: %s\n", client_name);
       printf("client pass: %s\n", client_pass);
       printf("Server ip: %s\n", server_address);
       printf("server port: %s\n", server_p);


       msg.size = strlen(client_pass);
       strncat(msg.source, client_name, 100);
       strncat(msg.data, client_pass,100);   

    // At this point the first login message is ready to send 

    // Now we try to connect to the server with the given IP and port
    // if we fail we return back to the top and ask for new IP address and port

    memset(&hints, 0, sizeof(hints));
    // filling up some information about the server

    hints.ai_family = AF_INET;      // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_STREAM; // for a TCP connection

    server_p[strcspn(server_p, "\n")] = 0; // to remove the new line character at the end of the server_p buffer

    if ((rv = getaddrinfo(server_address,server_p, &hints, &servinfo)) != 0)
    { // here we get the infomration based on the IP address and port number given
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        continue;
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
            perror("client: connect failure");
            continue;
        }
            break;
        }

    // in case we couldnt make a socket out of the IP address and port number given
    if (p == NULL)
    {
        fprintf(stderr, "Failed to create socket and connect\n");
        continue;
    }
    // just to print the connection message
     if ( *inet_ntop(p->ai_family , (struct sockaddr *)p->ai_addr, s, sizeof (s)) == -1 ){
            perror("inet_ntop failed");
        }

    printf("client: connecting to %s\n", s);


    //convert the struct to a buffer ready to send 

    char login_buffer[4096] = {'\0'};
    char num_buffer[4096] = {'\0'};
    char colon_str[2] = ":";

    sprintf(num_buffer, "%d", msg.type);
    strcat(login_buffer, num_buffer);
    strcat(login_buffer, colon_str);

    sprintf(num_buffer, "%d", msg.size);
    strcat(login_buffer, num_buffer);
    strcat(login_buffer, colon_str);

    strcat(login_buffer, msg.source);
    strcat(login_buffer, colon_str);

    strcat(login_buffer, msg.data);

    int len_login_msg = strlen(login_buffer);

    printf("Sending to Server: %s\n",login_buffer);

    if (send(sockfd, login_buffer, len_login_msg, 0) == -1)
    {
        perror("send");
        close(sockfd);
        exit(0);
    }

    // At this point we need to wait for a reply and check if it is an ACK or a NACK

    int num_bytes = 0;
    char buf[MAXDATASIZE];

    if ((num_bytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == 0) 
    {
        close(sockfd);
        printf("closing connection\n");
        perror("recv fininshed");
        continue;
    }
    buf[num_bytes] = '\0';
    printf("Client : received : %s \n", buf);

    if (buf[0]== '2'){ //means ack was recieved
        printf("Client Authenticated and joined a connection\n");
    }
    else {
        printf("Client could not be authenicated, please try again\n");
        continue;
    }

    printf("Enter the message (or enter /logout to logout or /quit to exit the program ): \n");

    // we can define the simple query message now since it wont change

    // predifing the query message
    // defining the lo ack message so early because it doesnt change

    query.type = 12; // index based on the table in the document
    query.size = 0;

    strcat(query.source, client_name);
    strncat(query.data, zero, 2);

    char query_buffer[4096] = {'\0'};

    sprintf(num_buffer, "%d", query.type);
    strcat(query_buffer, num_buffer);
    strcat(query_buffer, colon_str);

    sprintf(num_buffer, "%d", query.size);
    strcat(query_buffer, num_buffer);
    strcat(query_buffer, colon_str);

    strcat(query_buffer, query.source);
    strcat(query_buffer, colon_str);

    strcat(query_buffer, query.data);

    int len_query_msg = strlen(query_buffer);

    while(1){
        // inside here I am logged in to the server 


        //once we are logged into the server we can add the sockfd into our read_set for multiplexing

        FD_SET(0,&read_fds ); //add stdin
        FD_SET(sockfd,&read_fds); // add the socket file descriptor



        //still need to implement joining a session as that is when we will actually be reading from stdin and sockfd,

        //for now i will simply implement it here. if there is data from the server i will print it in output, if i type anything in terminal 
        // i will send it to server

        char str[MAXDATASIZE];

         fgets(str, MAXDATASIZE, stdin);
            int len = strlen(str);

            char exit_msg[100] = "/logout\n";

            if (strcmp(str, exit_msg) == 0)
            {
                break;
            }

            if (strcmp(str, "/quit\n") == 0)
            { // the only way to exit the program
                printf("Terminating Program\n");
                quit = true;
                break;
            }

            if (strcmp(str, "/list\n") == 0)
            { // Ask for list from server

               printf("Querying server for list\n");
                if (send(sockfd, query_buffer, len_query_msg, 0) == -1)
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                char buf3[MAXDATASIZE];

                if ((num_bytes = recv(sockfd, buf3, MAXDATASIZE - 1, 0)) == 0)
                {
                    close(sockfd);
                    printf("closing connection\n");
                    perror("recv fininshed");
                    continue;
                }
                buf3[num_bytes] = '\0';

                //here i should recieve a QU_ack message il just print it for now
                printf("Client : received : %s \n", buf3);

                printf("Enter the message (or enter /logout to logout or /quit to exit the program ): \n");

                continue;
            }


        


        
        if (!in_session){
         printf("Cant send messages if not in session, please type /list to see the list of sessions or create your own.\n");
         printf("Enter the message (or enter /logout to logout or /quit to exit the program ): \n");

         continue;
        }




        while (in_session){


        if (select(sockfd+1, &read_fds, NULL, NULL, NULL) < 0){
            perror("select error");
            exit(1);
        }

        if (FD_ISSET(sockfd, &read_fds)){

            char buf2[MAXDATASIZE];

            if ((num_bytes = recv(sockfd, buf2, MAXDATASIZE - 1, 0)) == 0)
            {
                close(sockfd);
                printf("closing connection\n");
                perror("recv fininshed");
                continue;
            }
            buf2[num_bytes] = '\0';
            printf("Client : received : %s \n", buf2);
        }

        if (FD_ISSET(0, &read_fds))
        {

            fgets(str, MAXDATASIZE, stdin);
            int len = strlen(str);

            char exit_msg[100] = "/logout\n";

            if (strcmp(str, exit_msg) == 0)
            {
                break;
            }

            if (strcmp(str, "/quit\n") == 0)
            { // the only way to exit the program
                printf("Terminating Program\n");
                quit = true;
                break;
            }

            if (send(sockfd, str, len, 0) == -1)
            {
                perror("send");
                close(sockfd);
                exit(0);
            }

            printf("Enter the message (or enter /logout to logout or /quit to exit the program ): \n");
        }

    }

        }

    printf("closing connection \n");

    freeaddrinfo(servinfo);
    close(sockfd);
    }

}
