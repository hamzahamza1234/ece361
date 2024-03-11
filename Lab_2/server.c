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
#include <cassert>

#define BACKLOG 20
#define MAXDATASIZE 100
#define MAX_NAME 100
#define MAX_PSWRD 100
#define MAX_SESSION 100
#define NUM_USERS 5

struct message
{
    unsigned int type;
    unsigned int size;
    char source[MAX_NAME];
    char data[MAXDATASIZE];
};

typedef struct client_info
{
    char username[MAX_NAME];
    char password[MAX_PSWRD];
    bool logged_in;
    char cur_session[MAX_SESSION];
    int port_fd;
} Client;

typedef struct session_info
{
    char name[MAX_SESSION];
    int num_users;
    bool active;
    int size; // Length of name
} Session;


// Defining some predefined message 
struct message lo_ack;
struct message lo_nack;
struct message qu_ack;

bool in_session = false;

// List of usernames and passwords that we will use to authenticate the users of our network
Client client_list[NUM_USERS] = {
    {"Hamza", "12345", false, ""},
    {"Caitlin", "67890", false, ""},
    {"Guest", "asdf", false, ""},
    {"Bob", "123abc", false, ""},
    {"Sally123!", "ca$h", false, ""},
};

Session session_list[NUM_USERS] = {
    {"", 0, false, 0},
    {"", 0, false, 0},
    {"", 0, false, 0},
    {"", 0, false, 0},
    {"", 0, false, 0},
};

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

    fd_set read_fds; //for the select system call

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

        in_session = false;

        printf("looking for a new connection\n");

        if (listen(sockfd, BACKLOG) == -1){
            perror("listen failed");
            exit(1);
        }

        int max_fd = sockfd;
        //printf("========================\n");
        //printf("%d\n", sockfd);

        FD_ZERO(&read_fds);

        FD_SET(sockfd,&read_fds); // add the socket file descriptor
        // Adding all the active client file descriptors
        for (int i = 0; i < NUM_USERS; i++) {
            if (client_list[i].logged_in) {
                FD_SET(client_list[i].port_fd, &read_fds);
                if (client_list[i].port_fd > max_fd) {
                    max_fd = client_list[i].port_fd;
                }
                //printf("%d\n", client_list[i].port_fd);
            }
        }
        //printf("========================\n");

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select error");
            exit(1);
        }

        // Determining which file descriptor unblocked
        if (FD_ISSET(sockfd, &read_fds)) {
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

            bool valid_login = false;
            struct message msg = {0, 0, "", ""};

            // Converting the buffer to a message struct
            sscanf(buf, "%u:%u:", &msg.type, &msg.size);
                
            int name_start_index = 0, name_end_index = 0, num_colon = 0;
            for (int i = 0; i < num_bytes; i++) {
                if (buf[i] == ':') {
                    num_colon++;
                    if (num_colon == 2) {
                        name_start_index = i + 1;
                    }
                    if (num_colon == 3) {
                        name_end_index = i;
                    }
                }
            }
            memcpy(msg.source, &buf[name_start_index], name_end_index - name_start_index);
            memcpy(msg.data, &buf[name_end_index + 1], msg.size);

            assert (msg.type == 1); // TODO: remove this assert once confident that the only messages recieved here are of type 1

            // Authenticating user
            printf("MSG: source=%s data=%s\n", msg.source, msg.data);
            for (int i = 0; i < NUM_USERS; i++) {
                printf("CLIENT: username=%s, password=%s, logged in=%s\n", client_list[i].username, client_list[i].password,  client_list[i].logged_in ? "true" : "false");

                if ((strcmp(msg.source, client_list[i].username) == 0) 
                    && (strcmp(msg.data, client_list[i].password) == 0)
                    && !(client_list[i].logged_in)) {
                        valid_login = true;
                        client_list[i].logged_in = true;
                        client_list[i].cur_session[0] = '\0';
                        client_list[i].port_fd = new_fd;
                }
            }

            if (valid_login){ // need to check login message for authentication here (i simply did 1 for now)
                if (send(new_fd, lo_ack_buffer, len_lo_ack_msg, 0) == -1)
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                printf("Client has been authenicated and joined connection\n");
            } else {  //if authentication failed send a lo_nack message and continue searching for clients
                if (send(new_fd, lo_nack_buffer, len_lo_nack_msg, 0) == -1) // TODO: lo_nack needs to include the reason for failure
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }
                printf("Client Authenication Failed\n");
                continue;

            }
        } else {
            int cur_fd = -1;
            int client_index = -1;
            // Looping through the file descriptors of the active clients
            for (int i = 0; i < NUM_USERS; i++) {
                if (client_list[i].logged_in && FD_ISSET(client_list[i].port_fd, &read_fds)) {
                    cur_fd = client_list[i].port_fd;
                    client_index = i;
                }
            }

            assert(cur_fd!=-1 && client_index != -1); // TODO: Remove assert once confident

            int num_bytes = 0;
            if ((num_bytes = recv(cur_fd, buf, MAXDATASIZE - 1, 0)) == 0 )
            {
                // Find client to update info
                /*for (int i = 0; i < NUM_USERS; i++) {
                    if (client_list[i].port_fd == cur_fd) {
                        client_list[i].logged_in = false;
                    }
                }*/
                client_list[client_index].logged_in = false; // TODO: remove client from whatever server they are in
                close(cur_fd);
                printf("Client has closed connection\n");
                perror("recv finished");
                //break;
                continue;
            }
            buf[num_bytes] = '\0';
            printf("SERVER : received : %s \n", buf);

            struct message msg = {0, 0, "", ""};

            // Converting the buffer to a message struct
            sscanf(buf, "%u:%u:", &msg.type, &msg.size);
                
            int name_start_index = 0, name_end_index = 0, num_colon = 0;
            for (int i = 0; i < num_bytes; i++) {
                if (buf[i] == ':') {
                    num_colon++;
                    if (num_colon == 2) {
                        name_start_index = i + 1;
                    }
                    if (num_colon == 3) {
                        name_end_index = i;
                    }
                }
            }
            memcpy(msg.source, &buf[name_start_index], name_end_index - name_start_index);
            memcpy(msg.data, &buf[name_end_index + 1], msg.size);

            if (msg.type == 12) { // this means it is a query message

                for (int i = 0; i < NUM_USERS; i++) {
                    if (session_list[i].active) {
                        printf("%s\n", session_list[i].name);
                    }
                }
                for (int i = 0; i < NUM_USERS; i++) {
                    if (client_list[i].logged_in) {
                        printf("%s\n", client_list[i].username);
                    }
                }

                printf("Recieved Query  request from client. \n");
                printf("Sending QU_ACK back to client. \n");

                if (send(cur_fd, buf, num_bytes, 0) == -1)  //for now we will just send the message back but we have to implement sending users and sessions
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                continue;
            }

            if (msg.type == 5)
            { // this means it is a join session message

                printf("Recieved the join request from client. \n");
                printf("Sending JN_ack or JN_nack back to client. \n");

                bool joined_session = false;

                // Finding the requested session
                for (int i = 0; i < NUM_USERS; i++) {
                    if (session_list[i].active && (session_list[i].size == msg.size) && (strncmp(session_list[i].name, msg.data, msg.size) == 0)) {
                        session_list[i].num_users++;
                        strncpy(client_list[client_index].cur_session, msg.data, msg.size);
                        joined_session = true;
                    }
                }

                if (joined_session) {
                    // Send ACK
                } else { //TODO: specify reason for NACK
                    // Send NACK
                }

                if (send(cur_fd, buf, num_bytes, 0) == -1) // for now we will just send the message back but we have to implement session joining
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                continue;
            }

            if (msg.type == 9)
            { // this means it is a create session message

                printf("Recieved the create request from client. \n");
                printf("Sending NS_ack back to client. \n");

                bool created_session = false;

                // Find the first inactive session in the session list
                for (int i = 0; i < NUM_USERS; i++) {
                    if (!session_list[i].active) {
                        // Updating all necessary info
                        strncpy(session_list[i].name, msg.data, msg.size);
                        strncpy(client_list[client_index].cur_session, msg.data, msg.size);
                        session_list[i].active = true;
                        session_list[i].num_users = 1;
                        session_list[i].size = msg.size;
                        created_session = true;
                        break;
                    }
                }

                assert(created_session); // TODO: Remove this assert once confident

                //not much checking to do here because my client code can only send one create session message and only if it isnt already in one

                if (send(cur_fd, buf, num_bytes, 0) == -1) // for now we will just send the message back but we have to implement session creation
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                continue;
            }

            if (msg.type == 8)
            { // this means it is a leave session message

                printf("Recieved the leave request from client. \n");

                // Finding the session the client was in
                for (int i = 0; i < NUM_USERS; i++) {
                    if (session_list[i].active && (strcmp(session_list[i].name, client_list[client_index].cur_session) == 0)) {
                        session_list[i].num_users--;
                        if (session_list[i].num_users == 0) {
                            // Setting the session as inactive since the last client left
                            session_list[i].active = false;
                            session_list[i].name[0] = '\0';
                        }
                        client_list[client_index].cur_session[0] = '\0';
                    }
                }


                continue;
            }
        }

        /*if (listen(sockfd, BACKLOG) == -1){
            perror("listen failed");
            exit(1);
        }*/

        /*new_fd = accept(sockfd, (struct sockaddr*) &from_addr, &addr_size);

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

        bool valid_login = false;
        struct message msg;

        // Converting the buffer to a message struct
        sscanf(buf, "%u:%u:", &msg.type, &msg.size);
            
        int name_start_index = 0, name_end_index = 0, num_colon = 0;
        for (int i = 0; i < num_bytes; i++) {
            if (buf[i] == ':') {
                num_colon++;
                if (num_colon == 2) {
                    name_start_index = i + 1;
                }
                if (num_colon == 3) {
                    name_end_index = i;
                }
            }
        }
        memcpy(msg.source, &buf[name_start_index], name_end_index - name_start_index);
        memcpy(msg.data, &buf[name_end_index + 1], msg.size);

        /*printf("MESSAGE\n");
        printf("type:%u size:%u name:%s data: %s\n", msg.type, msg.size, msg.source, msg.data);*/

        // Authenticating user
        /*for (int i = 0; i < NUM_USERS; i++) {
            if ((strcmp(msg.source, client_list[i].username) == 0) 
                && (strcmp(msg.data, client_list[i].password) == 0)
                && !(client_list[i].logged_in)) {
                    valid_login = true;
                    client_list[i].logged_in = true;
                    client_list[i].cur_session[0] = '\0';
                    client_list[i].port_fd = new_fd;
            }
        }

        if (valid_login){ // need to check login message for authentication here (i simply did 1 for now)
            if (send(new_fd, lo_ack_buffer, len_lo_ack_msg, 0) == -1)
            {
                perror("send");
                close(sockfd);
                exit(0);
            }

            printf("Client has been authenicated and joined connection\n");
        } else {  //if authentication failed send a lo_nack message and continue searching for clients
            if (send(new_fd, lo_nack_buffer, len_lo_nack_msg, 0) == -1) // TODO: lo_nack needs to include the reason for failure
            {
                perror("send");
                close(sockfd);
                exit(0);
            }
            printf("Client Authenication Failed\n");
            continue;

        }*/

        // this loop makes sure we keep listening to our client until it closes and echoing the message back
        // I have added this echo to make sure my client is able to both read from server and stdin at the same time

        /*while(num_bytes != 0){  
            num_bytes = 0;
            if ((num_bytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == 0 )
            {
                // Find client to update info
                for (int i = 0; i < NUM_USERS; i++) {
                    if (client_list[i].port_fd == new_fd) {
                        client_list[i].logged_in = false;
                    }
                }
                close(new_fd);
                printf("Client has closed connection\n");
                perror("recv finished");
                break;
            }
            buf[num_bytes] = '\0';
            printf("SERVER : received : %s \n", buf);



            if (buf[0] == '1' && buf[1] == '2') { // this means it is a query message

                printf("Recieved Query  request from client. \n");
                printf("Sending QU_ACK back to client. \n");

                if (send(new_fd, buf, num_bytes, 0) == -1)  //for now we will just send the message back but we have to implement sending users and sessions
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                continue;
            }

            if (buf[0] == '5')
            { // this means it is a join session message

               printf("Recieved the join request from client. \n");
               printf("Sending JN_ack or JN_nack back to client. \n");

                if (send(new_fd, buf, num_bytes, 0) == -1) // for now we will just send the message back but we have to implement session joining
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                continue;
            }

            if (buf[0] == '9')
            { // this means it is a create session message

                printf("Recieved the create request from client. \n");
                printf("Sending NS_ack back to client. \n");

                //not much checking to do here because my client code can only send one create session message and only if it isnt already in one

                if (send(new_fd, buf, num_bytes, 0) == -1) // for now we will just send the message back but we have to implement session creation
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                continue;
            }

            if (buf[0] == '8')
            { // this means it is a leave session message

                printf("Recieved the leave request from client. \n");

                //add whatever needed to make sure client isnt in session

                continue;
            }
        }*/


        
    }


    return 0;
}