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
bool logout = false;
bool invited = false;

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

        logout = false;

        FD_ZERO(&read_fds); // make sure its zerod out before every new connection

        if (quit == true){
            break;
        }

        char command[4096];
        char login_com[4096];
        
        struct message msg = {0, 0, "", ""};   //this struct will store the first login_msg sent to server

        printf("New Client has started, Please enter a command\n");
        fgets(command, 4096, stdin);  //using fgets because scanf terminates after whitespaces

        if (strlen(command) == 1){
            printf("Empty space is not allowed. \n");
            continue;
        }
        if (strlen(command) > MAXDATASIZE){
            printf("Please Remain within 100 characters.\n");
            continue;
        }

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
       strncat(msg.source, client_name, 100); // TODO: Should these be switched to strncpy()?
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
        struct message msg_nack= {0, 0, "", ""};

        // Converting the buffer to a message struct
        sscanf(buf, "%u:%u:", &msg_nack.type, &msg_nack.size);

        int name_start_index = 0, name_end_index = 0, num_colon = 0;
        for (int i = 0; i < num_bytes; i++)
        {
            if (buf[i] == ':')
            {
                num_colon++;
                if (num_colon == 2)
                {
                    name_start_index = i + 1;
                }
                if (num_colon == 3)
                {
                    name_end_index = i;
                }
            }
        }
        memcpy(msg_nack.source, &buf[name_start_index], name_end_index - name_start_index);
        memcpy(msg_nack.data, &buf[name_end_index + 1], msg_nack.size);

        printf("Could Not Login:\n");
        printf("%s \n", msg_nack.data);

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

        if (quit || logout){
            break;
        }

        FD_ZERO(&read_fds);

        FD_SET(STDIN, &read_fds);  // add stdin
        FD_SET(sockfd, &read_fds); // add the socket file descriptor


        if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror("select error");
            exit(1);
        }

        if (FD_ISSET(sockfd, &read_fds))
        {
            char buf7[MAXDATASIZE];

            if ((num_bytes = recv(sockfd, buf7, MAXDATASIZE - 1, 0)) == 0)
            {
                close(sockfd);
                printf("closing connection\n");
                perror("recv fininshed");
                continue;
            }
            buf7[num_bytes] = '\0';

            struct message msg_chat_1 = {0, 0, "", ""};

            // Converting the buffer to a message struct
            sscanf(buf7, "%u:%u:", &msg_chat_1.type, &msg_chat_1.size);

            int name_start_index = 0, name_end_index = 0, num_colon = 0;
            for (int i = 0; i < num_bytes; i++)
            {
                if (buf7[i] == ':')
                {
                    num_colon++;
                    if (num_colon == 2)
                    {
                        name_start_index = i + 1;
                    }
                    if (num_colon == 3)
                    {
                        name_end_index = i;
                    }
                }
            }
            memcpy(msg_chat_1.source, &buf7[name_start_index], name_end_index - name_start_index);
            memcpy(msg_chat_1.data, &buf7[name_end_index + 1], msg_chat_1.size);

            if (msg_chat_1.type == 14)
            { // invited recieved
                printf("Invite recieved from %s to join %s.\n", msg_chat_1.source, msg_chat_1.data);
                printf("Either type /accept with session name or /decline to decline\n");
                invited = true;
                continue;
            }

            continue;
        }

        if (FD_ISSET(STDIN, &read_fds)){

            char str[MAXDATASIZE];

            fgets(str, 4096, stdin);
            int len = strlen(str);
        if (len  == 1){
            printf("Empty space is not allowed. \n");
            continue;
        }
        if (len > MAXDATASIZE){
            printf("Please Remain within 100 characters.\n");
            continue;
        }

             str[strcspn(str, "\n")] = 0; // to remove the new line character

            char exit_msg[100] = "/logout";
            char join_ses_msg[100] = "/joinsession ";
            char create_ses_msg[100] = "/createsession ";

            if (strcmp(str, exit_msg) == 0)
            {
                break;
            }

            if (strcmp(str, "/quit") == 0)
            { // the only way to exit the program
                printf("Terminating Program\n");
                quit = true;
                break;
            }



            if (strcmp(str, "/list") == 0)
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

                struct message msg_query = {0, 0, "", ""};

                // Converting the buffer to a message struct
                sscanf(buf3, "%u:%u:", &msg_query.type, &msg_query.size);

                int name_start_index = 0, name_end_index = 0, num_colon = 0;
                for (int i = 0; i < num_bytes; i++)
                {
                    if (buf3[i] == ':')
                    {
                        num_colon++;
                        if (num_colon == 2)
                        {
                            name_start_index = i + 1;
                        }
                        if (num_colon == 3)
                        {
                            name_end_index = i;
                        }
                    }
                }
                memcpy(msg_query.source, &buf3[name_start_index], name_end_index - name_start_index);
                memcpy(msg_query.data, &buf3[name_end_index + 1], msg_query.size);

                printf("The List :\n");
                printf("%s", msg_query.data);

                printf("Enter the message (or enter /logout to logout or /quit to exit the program ): \n");

                continue;
            }

            // check for accept or decline
            // check if its an invite
            char accept_msg_check[100] = "/accept";

            if (strncmp(str, accept_msg_check,7) == 0)
            { // this means its an accept

                // lets make the accept message and send it

                if (!invited)
                {
                    printf("You Havent been invited anywhere.\n");
                    continue;
                }

                struct message accept_msg = {0, 0, "", ""};

                accept_msg.type = 15;
                strcat(accept_msg.source, client_name);
                strncat(accept_msg.data, str + 8, 100);
                accept_msg.size = strlen(accept_msg.data);

                if (accept_msg.size == 0)
                {
                    printf("Please Enter /accept with the name of session to accept invite.\n");
                    continue;
                }

                printf("Accepted the invite to join %s. \n", accept_msg.data);

                char accept_buffer[4096] = {'\0'};

                sprintf(num_buffer, "%d", accept_msg.type);
                strcat(accept_buffer, num_buffer);
                strcat(accept_buffer, colon_str);

                sprintf(num_buffer, "%d", accept_msg.size);
                strcat(accept_buffer, num_buffer);
                strcat(accept_buffer, colon_str);

                strcat(accept_buffer, accept_msg.source);
                strcat(accept_buffer, colon_str);

                strcat(accept_buffer, accept_msg.data);

                int len_accept_msg = strlen(accept_buffer);

                printf("Sending to server: %s\n", accept_buffer);

                if (send(sockfd, accept_buffer, len_accept_msg, 0) == -1)
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                invited = false; // since we already accept it
                char buf8[MAXDATASIZE];

                if ((num_bytes = recv(sockfd, buf8, MAXDATASIZE - 1, 0)) == 0)
                {
                    close(sockfd);
                    printf("closing connection\n");
                    perror("recv fininshed");
                    continue;
                }
                buf8[num_bytes] = '\0';

                // here i should recieve a JN_ack or JN_nack message il just print it for now
                printf("Client : received : %s \n", buf8);

                if (buf8[0] == '6')
                { // will implement checking the jn ack and jn nack message here (just 1 for now)
                    in_session = true;
                    printf("Session Joined.\n");
                }
                else
                {
                    printf("join session failed, session doesnt exist. Please Try again.\n");
                    printf("Enter the message (or enter /logout to logout or /quit to exit the program ): \n");
                    continue;
                }
            }

            // check if its a decline
            char decline_msg_check[100] = "/decline";


            if (strcmp(str, decline_msg_check) == 0)
            { // this means its a decline

                // lets make the decline message and send it

                if (!invited)
                {
                    printf("You Havent been invited anywhere.\n");
                    continue;
                }

                struct message decline_msg = {0, 0, "", ""};

                decline_msg.type = 16;
                decline_msg.size = 0;
                strcat(decline_msg.source, client_name);
                strncat(decline_msg.data, zero, 2);

                printf("Declined the invitation.\n");

                char decline_buffer[4096] = {'\0'};

                sprintf(num_buffer, "%d", decline_msg.type);
                strcat(decline_buffer, num_buffer);
                strcat(decline_buffer, colon_str);

                sprintf(num_buffer, "%d", decline_msg.size);
                strcat(decline_buffer, num_buffer);
                strcat(decline_buffer, colon_str);

                strcat(decline_buffer, decline_msg.source);
                strcat(decline_buffer, colon_str);

                strcat(decline_buffer, decline_msg.data);

                int len_decline_msg = strlen(decline_buffer);

                printf("Sending to server: %s\n", decline_buffer);

                if (send(sockfd, decline_buffer, len_decline_msg, 0) == -1)
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                invited = false; // since we already declined it
                continue;
            }

            //lets add priv function

            char priv_msg_check[100] = "/priv";

            if (strncmp(str, priv_msg_check, 5) == 0)
            { // this means its a priv message

                // lets make the priv message and send it

                struct message priv_msg = {0, 0, "", ""};

                priv_msg.type = 17;
                strcat(priv_msg.source, client_name);
                strncat(priv_msg.data, str + 6, 100);
                priv_msg.size = strlen(priv_msg.data);

                if (priv_msg.size == 0)
                {
                    printf("please enter the name and message to send with /priv\n");
                    continue;
                }

                char delim[] = " "; // used to seperate the login command to all the different arguments
                int num_args = 0;
                char priv_com[MAXDATASIZE];
                strncpy(priv_com, str + 6, MAXDATASIZE);

                char *priv_ptr = strtok(priv_com, delim);
                char send_name[100];
                while (priv_ptr != NULL)
                { // this saves each argument in its respective place
                    if (num_args == 0)
                    {
                        strncpy(send_name, priv_ptr, 100);
                    }

                    num_args++;
                    priv_ptr = strtok(NULL, delim);
                }

                if (num_args < 2)
                { // error checking for too many arguments
                    printf("Wrong number of Arguments\n");
                    continue;
                }

                printf("Sending Private message to %s.\n",send_name);

                char priv_buffer[4096] = {'\0'};

                sprintf(num_buffer, "%d", priv_msg.type);
                strcat(priv_buffer, num_buffer);
                strcat(priv_buffer, colon_str);

                sprintf(num_buffer, "%d", priv_msg.size);
                strcat(priv_buffer, num_buffer);
                strcat(priv_buffer, colon_str);

                strcat(priv_buffer, priv_msg.source);
                strcat(priv_buffer, colon_str);

                strcat(priv_buffer, priv_msg.data);

                int len_priv_msg = strlen(priv_buffer);

                printf("Sending to server: %s\n", priv_buffer);

                if (send(sockfd, priv_buffer, len_priv_msg, 0) == -1)
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }
                continue;
            }

            // lets check if they want to do sessional commands

            char sess_command[MAXDATASIZE];

            strncpy(sess_command, str,13 );
            if (strcmp(sess_command, join_ses_msg) == 0){
                printf("Client wants to join a session \n");

                //lets make the join message and send it

                struct message join_msg = {0, 0, "", ""};

                join_msg.type = 5;
                strcat(join_msg.source, client_name);
                strncat(join_msg.data, str + 13, 100);
                join_msg.size = strlen(join_msg.data);

                char join_buffer[4096] = {'\0'};

                sprintf(num_buffer, "%d", join_msg.type);
                strcat(join_buffer, num_buffer);
                strcat(join_buffer, colon_str);

                sprintf(num_buffer, "%d", join_msg.size);
                strcat(join_buffer, num_buffer);
                strcat(join_buffer, colon_str);

                strcat(join_buffer, join_msg.source);
                strcat(join_buffer, colon_str);

                strcat(join_buffer, join_msg.data);

                int len_join_msg = strlen(join_buffer);

                printf ("Sending to server: %s\n", join_buffer);

                if (send(sockfd, join_buffer, len_join_msg, 0) == -1)
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                char buf4[MAXDATASIZE];

                if ((num_bytes = recv(sockfd, buf4, MAXDATASIZE - 1, 0)) == 0)
                {
                    close(sockfd);
                    printf("closing connection\n");
                    perror("recv fininshed");
                    continue;
                }
                buf4[num_bytes] = '\0';

                // here i should recieve a JN_ack or JN_nack message il just print it for now
                printf("Client : received : %s \n", buf4);

                if (buf4[0] == '6'){ // will implement checking the jn ack and jn nack message here (just 1 for now)
                  in_session = true;
                  printf("Session Joined.\n");
                }else{
                    printf("join session failed, session doesnt exist. Please Try again.\n");
                printf("Enter the message (or enter /logout to logout or /quit to exit the program ): \n");
                continue;
                }

            }
            else{
                char sess_command2[MAXDATASIZE];
            strncpy(sess_command2, str,14 );
            if (strcmp(sess_command, "/leavesession") == 0){
                printf("Cant leave session without joining one, Try Again\n");
                continue;
            }
            else{
                strncpy(sess_command, str, 15);
                if (strcmp(sess_command, create_ses_msg) == 0)
                {
                    printf("Client wants to create a session \n");

                    // lets make the create message and send it

                    struct message create_msg = {0, 0, "", ""};

                    create_msg.type = 9; //code for create message
                    strcat(create_msg.source, client_name);
                    strncat(create_msg.data, str + 15, 100);
                    create_msg.size = strlen(create_msg.data);

                    char create_buffer[4096] = {'\0'};

                    sprintf(num_buffer, "%d", create_msg.type);
                    strcat(create_buffer, num_buffer);
                    strcat(create_buffer, colon_str);

                    sprintf(num_buffer, "%d", create_msg.size);
                    strcat(create_buffer, num_buffer);
                    strcat(create_buffer, colon_str);

                    strcat(create_buffer, create_msg.source);
                    strcat(create_buffer, colon_str);

                    strcat(create_buffer, create_msg.data);

                    int len_create_msg = strlen(create_buffer);

                    printf("Sending to server: %s\n", create_buffer);

                    if (send(sockfd, create_buffer, len_create_msg, 0) == -1)
                    {
                        perror("send");
                        close(sockfd);
                        exit(0);
                    }

                    char buf5[MAXDATASIZE];

                    if ((num_bytes = recv(sockfd, buf5, MAXDATASIZE - 1, 0)) == 0)
                    {
                        close(sockfd);
                        printf("closing connection\n");
                        perror("recv fininshed");
                        continue;
                    }
                    buf5[num_bytes] = '\0';

                    // here i should recieve a NS_ACK message il just print it for now
                    printf("Client : received : %s \n", buf5);

                    if (1)
                    { // will implement checking the NS_ack message checking here
                        in_session = true;
                        printf("Session Created. \n");
                    }
                    else
                    {
                        printf("Enter the message (or enter /logout to logout or /quit to exit the program ): \n");
                        continue;
                    }
                }
            }
            }


            if (!in_session)
            {
                printf("Cant send messages if not in session, please type /list to see the list of sessions or create your own.\n");
                printf("Enter the message (or enter /logout to logout or /quit to exit the program ): \n");

                continue;
        }
    }

        
        // This will be last and final loop in which we will be in a session sending messages from stdin and recieving messages from server
        while (in_session){
            FD_ZERO(&read_fds);

            FD_SET(STDIN, &read_fds);  // add stdin
            FD_SET(sockfd, &read_fds); // add the socket file descriptor

            printf("Enter the message to send or enter /leavesession to leave the session (or any relevant command) \n");

            if (select(sockfd + 1, &read_fds, NULL, NULL, NULL) < 0)
            {
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
         //   printf("Client : received : %s \n", buf2);

            struct message msg_chat = {0, 0, "", ""};

            // Converting the buffer to a message struct
            sscanf(buf2, "%u:%u:", &msg_chat.type, &msg_chat.size);

            int name_start_index = 0, name_end_index = 0, num_colon = 0;
            for (int i = 0; i < num_bytes; i++)
            {
                if (buf2[i] == ':')
                {
                    num_colon++;
                    if (num_colon == 2)
                    {
                        name_start_index = i + 1;
                    }
                    if (num_colon == 3)
                    {
                        name_end_index = i;
                    }
                }
            }
            memcpy(msg_chat.source, &buf2[name_start_index], name_end_index - name_start_index);
            memcpy(msg_chat.data, &buf2[name_end_index + 1], msg_chat.size);

            if (msg_chat.type == 14)
            { // invited recieved while in session 
               printf("cant recieve invites while in session.\n");
               continue;
            }

            printf("Client Recieved a message from %s: %s \n",msg_chat.source, msg_chat.data);
        }

        if (FD_ISSET(STDIN, &read_fds))
        {
            char str[MAXDATASIZE];
            fgets(str, MAXDATASIZE, stdin);
            
            int len = strlen(str);

        if (len  == 1){
            printf("Empty space is not allowed. \n");
            continue;
        }
        if (len  > MAXDATASIZE){
            printf("Please Remain within 100 characters.\n");
            continue;
        }

            str[strcspn(str, "\n")] = 0; // to remove the new line character

            char exit_msg[100] = "/logout";
            

            if (strcmp(str, exit_msg) == 0)
            {
                in_session = false;
                logout = true;
                break;
            }

            if (strcmp(str, "/quit") == 0)
            { // the only way to exit the program
                printf("Terminating Program\n");
                quit = true;
                break;
            }

            char sess_command[MAXDATASIZE];

            strncpy(sess_command, str, 14);
            if (strcmp(sess_command, "/leavesession") == 0)
            {
                printf("Leaving Session\n");
                in_session = false;

                //lets build the leave session message

                struct message leave_sess_msg = {0, 0, "", ""};
                leave_sess_msg.type = 8; // number for leave session
                leave_sess_msg.size = 0;

                strcat(leave_sess_msg.source, client_name);
                strncat(leave_sess_msg.data, zero, 2);

                char leave_buffer[4096] = {'\0'};

                sprintf(num_buffer, "%d", leave_sess_msg.type);
                strcat(leave_buffer, num_buffer);
                strcat(leave_buffer, colon_str);

                sprintf(num_buffer, "%d", leave_sess_msg.size);
                strcat(leave_buffer, num_buffer);
                strcat(leave_buffer, colon_str);

                strcat(leave_buffer, query.source);
                strcat(leave_buffer, colon_str);

                strcat(leave_buffer, query.data);

                int len_leave_msg = strlen(leave_buffer);

                printf("Sending to server : %s\n", leave_buffer);

                if (send(sockfd, leave_buffer, len_leave_msg, 0) == -1)
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                break;
            }

            if (strcmp(str, "/list") == 0)
            { // Ask for list from server

                printf("Querying server for list\n");
                if (send(sockfd, query_buffer, len_query_msg, 0) == -1)
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                char buf3[MAXDATASIZE];

                if ((num_bytes = recv(sockfd, buf3, 4096, 0)) == 0)
                {
                    close(sockfd);
                    printf("closing connection\n");
                    perror("recv fininshed");
                    continue;
                }
                buf3[num_bytes] = '\0';

                struct message msg_query = {0, 0, "", ""};

                // Converting the buffer to a message struct
                sscanf(buf3, "%u:%u:", &msg_query.type, &msg_query.size);

                int name_start_index = 0, name_end_index = 0, num_colon = 0;
                for (int i = 0; i < num_bytes; i++)
                {
                    if (buf3[i] == ':')
                    {
                        num_colon++;
                        if (num_colon == 2)
                        {
                            name_start_index = i + 1;
                        }
                        if (num_colon == 3)
                        {
                            name_end_index = i;
                        }
                    }
                }
                memcpy(msg_query.source, &buf3[name_start_index], name_end_index - name_start_index);
                memcpy(msg_query.data, &buf3[name_end_index + 1], msg_query.size);

                printf("The List :\n");
                printf("%s", msg_query.data);


                continue;
            }
            // lets add priv function

            char priv_msg_check[100] = "/priv";

            if (strncmp(str, priv_msg_check, 5) == 0)
            { // this means its a priv message

                // lets make the priv message and send it

                struct message priv_msg = {0, 0, "", ""};

                priv_msg.type = 17;
                strcat(priv_msg.source, client_name);
                strncat(priv_msg.data, str + 6, 100);
                priv_msg.size = strlen(priv_msg.data);

                if (priv_msg.size == 0)
                {
                    printf("please enter the name and message to send with /priv\n");
                    continue;
                }

                char delim[] = " "; // used to seperate the login command to all the different arguments
                int num_args = 0;
                char priv_com[MAXDATASIZE];
                strncpy(priv_com, str + 6, MAXDATASIZE);

                char *priv_ptr = strtok(priv_com, delim);
                char send_name[100];
                while (priv_ptr != NULL)
                { // this saves each argument in its respective place
                    if (num_args == 0)
                    {
                        strncpy(send_name, priv_ptr, 100);
                    }

                    num_args++;
                    priv_ptr = strtok(NULL, delim);
                }

                if (num_args < 2)
                { // error checking for too many arguments
                    printf("Wrong number of Arguments\n");
                    continue;
                }

                printf("Sending Private message to %s.\n", send_name);

                char priv_buffer[4096] = {'\0'};

                sprintf(num_buffer, "%d", priv_msg.type);
                strcat(priv_buffer, num_buffer);
                strcat(priv_buffer, colon_str);

                sprintf(num_buffer, "%d", priv_msg.size);
                strcat(priv_buffer, num_buffer);
                strcat(priv_buffer, colon_str);

                strcat(priv_buffer, priv_msg.source);
                strcat(priv_buffer, colon_str);

                strcat(priv_buffer, priv_msg.data);

                int len_priv_msg = strlen(priv_buffer);

                printf("Sending to server: %s\n", priv_buffer);

                if (send(sockfd, priv_buffer, len_priv_msg, 0) == -1)
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }
                continue;
            }

            //check if its an invite
            char inv_msg_check[100] = "/invite";
            char inv_command[MAXDATASIZE];


            if (strncmp(str, inv_msg_check,7) == 0){// this means its an invite

                // lets make the join message and send it

                struct message inv_msg = {0, 0, "", ""};

                inv_msg.type = 14;
                strcat(inv_msg.source, client_name);
                strncat(inv_msg.data, str + 8, 100);
                inv_msg.size = strlen(inv_msg.data);

                if (inv_msg.size == 0){
                    printf("Please Enter name of client to invite.\n");
                    continue;
                }

                printf("Invited %s to join session. \n", inv_msg.data);

                char inv_buffer[4096] = {'\0'};

                sprintf(num_buffer, "%d", inv_msg.type);
                strcat(inv_buffer, num_buffer);
                strcat(inv_buffer, colon_str);

                sprintf(num_buffer, "%d", inv_msg.size);
                strcat(inv_buffer, num_buffer);
                strcat(inv_buffer, colon_str);

                strcat(inv_buffer, inv_msg.source);
                strcat(inv_buffer, colon_str);

                strcat(inv_buffer, inv_msg.data);

                int len_inv_msg = strlen(inv_buffer);

                printf("Sending to server: %s\n", inv_buffer);

                if (send(sockfd, inv_buffer, len_inv_msg, 0) == -1)
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }
                continue;
            }


            //struct

            struct message chat_data = {0, 0, "", ""};

            chat_data.type = 11; //for message
            chat_data.size = len;

            strcat(chat_data.source, client_name);
            strncat(chat_data.data, str, 100);

            char message_data_buffer[4096] = {'\0'};

            sprintf(num_buffer, "%d", chat_data.type);
            strcat(message_data_buffer, num_buffer);
            strcat(message_data_buffer, colon_str);

            sprintf(num_buffer, "%d", chat_data.size);
            strcat(message_data_buffer, num_buffer);
            strcat(message_data_buffer, colon_str);

            strcat(message_data_buffer, chat_data.source);
            strcat(message_data_buffer, colon_str);

            strcat(message_data_buffer, chat_data.data);

            int main_len = strlen(message_data_buffer);

            if (send(sockfd, message_data_buffer, main_len, 0) == -1)
            {
                perror("send");
                close(sockfd);
                exit(0);
            }

        }

    }
    if (!quit && !logout ){
    printf("Enter the message (or enter /logout to logout or /quit to exit the program ): \n");
    }
        }

    printf("closing connection \n");

    freeaddrinfo(servinfo);
    close(sockfd);
    }

}
