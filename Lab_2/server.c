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
#define MAXDATASIZE 4096
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
struct message p_valid;
struct message p_invalid;


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

char colon_str[2] = ":";

/*
 *  send_message() - converts type, data_size, source, and data into a struct msg then converts the struct into a buffer and sends to fd
 *      - Returns the result of send
 */
int send_message(int fd, unsigned int type, unsigned int data_size, int src_size, char *source, char *data) {
    // Generating message
    struct message msg = {0, 0, "", ""};

    msg.type = type;
    msg.size = data_size;
    strncat(msg.source, source, src_size);
    strncpy(msg.data, data, data_size);
    

    
    char msg_buffer[4096] = {'\0'};
    char number_buffer[4096] = {'\0'};

    sprintf(number_buffer, "%d", msg.type);
    strcat(msg_buffer, number_buffer);
    strcat(msg_buffer, colon_str);

    sprintf(number_buffer, "%d", msg.size);
    strcat(msg_buffer, number_buffer);
    strcat(msg_buffer, colon_str);

    strcat(msg_buffer, msg.source);
    strcat(msg_buffer, colon_str);

    strcat(msg_buffer, msg.data);

    int len_msg = strlen(msg_buffer);

    //printf("Sending message: %s\n", msg_buffer);

    return send(fd, msg_buffer, len_msg, 0);
}

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
    //char colon_str[2] = ":";

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

    // defining the p_valid message

    p_valid.type = 18; // index based on the table in the document
    p_valid.size = 0;

    strncat(p_valid.source, zero, 2);
    strncat(p_valid.data, zero, 2);

    char p_valid_buffer[4096] = {'\0'};

    sprintf(num_buffer, "%d", p_valid.type);
    strcat(p_valid_buffer, num_buffer);
    strcat(p_valid_buffer, colon_str);

    sprintf(num_buffer, "%d", p_valid.size);
    strcat(p_valid_buffer, num_buffer);
    strcat(p_valid_buffer, colon_str);

    strcat(p_valid_buffer, p_valid.source);
    strcat(p_valid_buffer, colon_str);

    strcat(p_valid_buffer, lo_ack.data);

    int len_p_valid_msg = strlen(p_valid_buffer);

    // defining the p_invalid message

    p_invalid.type = 19; // index based on the table in the document
    p_invalid.size = 0;

    strncat(p_invalid.source, zero, 2);
    strncat(p_invalid.data, zero, 2);

    char p_invalid_buffer[4096] = {'\0'};

    sprintf(num_buffer, "%d", p_invalid.type);
    strcat(p_invalid_buffer, num_buffer);
    strcat(p_invalid_buffer, colon_str);

    sprintf(num_buffer, "%d", p_invalid.size);
    strcat(p_invalid_buffer, num_buffer);
    strcat(p_invalid_buffer, colon_str);

    strcat(p_invalid_buffer, p_invalid.source);
    strcat(p_invalid_buffer, colon_str);

    strcat(p_invalid_buffer, lo_ack.data);

    int len_p_invalid_msg = strlen(p_invalid_buffer);

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

        FD_ZERO(&read_fds);

        FD_SET(sockfd,&read_fds); // add the socket file descriptor
        // Adding all the active client file descriptors
        for (int i = 0; i < NUM_USERS; i++) {
            if (client_list[i].logged_in) {
                FD_SET(client_list[i].port_fd, &read_fds);
                if (client_list[i].port_fd > max_fd) {
                    max_fd = client_list[i].port_fd;
                }
            }
        }

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

            bool found_user = false;
            bool wrong_password = true;
            // Authenticating user
            for (int i = 0; i < NUM_USERS; i++) {
                if (strcmp(msg.source, client_list[i].username) == 0) {
                    found_user = true;
                    if (strcmp(msg.data, client_list[i].password) == 0) {
                        wrong_password = false;
                        if (!(client_list[i].logged_in)) {
                            valid_login = true;
                            client_list[i].logged_in = true;
                            client_list[i].cur_session[0] = '\0';
                            client_list[i].port_fd = new_fd;
                        }
                    }
                }
            }

            if (valid_login) { // Check login message for authentication 
                if (send(new_fd, lo_ack_buffer, len_lo_ack_msg, 0) == -1)
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }

                printf("Client has been authenicated and joined connection\n");
            } else {  //if authentication failed send a lo_nack message and continue searching for clients
                char nak_reason[100] = {'\0'};
                int nak_size = 0;
                if(!found_user) {
                    strncpy(nak_reason, "User doesn't exist.", 19);
                    nak_size = 19;
                } else if (wrong_password) {
                    strncpy(nak_reason, "Wrong password.", 15);
                    nak_size = 15;
                } else {
                    strncpy(nak_reason, "Already logged in.", 18);
                    nak_size = 18;
                }

                if (send_message(new_fd, 3, nak_size, 2, zero, nak_reason) == -1)
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
                client_list[client_index].logged_in = false;
                for (int i = 0; i < NUM_USERS; i++) { 
                    if(session_list[i].active && (strcmp(session_list[i].name, client_list[client_index].cur_session) == 0)) {
                        session_list[i].num_users--;
                        if (session_list[i].num_users == 0) {
                            // Setting the session as inactive since the last client left
                            session_list[i].active = false;
                            session_list[i].name[0] = '\0';
                        }
                        client_list[client_index].cur_session[0] = '\0';
                    }
                }
                close(cur_fd);
                printf("Client has closed connection\n");
                perror("recv finished");
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
                int num_char = 0;
                char qu_data_buffer[MAXDATASIZE] = {'\0'};

                // Adding the session header
                strncpy(qu_data_buffer, "SESSIONS\n", 9);
                num_char = 9;

                printf("Recieved Query  request from client. \n");
                printf("Sending QU_ACK back to client. \n");

                // Adding sessions to the message
                for (int i = 0; i < NUM_USERS; i++) {
                    if (session_list[i].active) {
                        strncat(qu_data_buffer, session_list[i].name, session_list[i].size);
                        strncat(qu_data_buffer, "\n", 2);
                        num_char += session_list[i].size + 1;
                        printf("%s\n", session_list[i].name);
                    }
                }
                // Adding the user header
                strncat(qu_data_buffer, "\nUSERS\n", 8);
                num_char += 7;


                // Adding the users to the message
                for (int i = 0; i < NUM_USERS; i++) {
                    if (client_list[i].logged_in) {
                        strncat(qu_data_buffer, client_list[i].username, strlen(client_list[i].username));
                        strncat(qu_data_buffer, "\n", 2);
                        num_char += strlen(client_list[i].username) + 1;
                        printf("%s\n", client_list[i].username);
                    }
                }

                if (send_message(cur_fd, 13, num_char, 2, zero, qu_data_buffer) == -1)
                {
                    perror("send");
                    close(sockfd);
                    exit(0);
                }
                printf("Sent QU_ACK to client.\n");
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
                    // Generating ACK message
                    struct message jn_ACK = {0, 0, "", ""};

                    jn_ACK.type = 6;
                    jn_ACK.size = msg.size;
                    strncat(jn_ACK.source, zero, 2);
                    strncpy(jn_ACK.data, msg.data, msg.size);

                    
                    char jn_ack_buffer[4096] = {'\0'};
                    char number_buffer[4096] = {'\0'};

                    sprintf(number_buffer, "%d", jn_ACK.type);
                    strcat(jn_ack_buffer, number_buffer);
                    strcat(jn_ack_buffer, colon_str);

                    sprintf(number_buffer, "%d", jn_ACK.size);
                    strcat(jn_ack_buffer, number_buffer);
                    strcat(jn_ack_buffer, colon_str);

                    strcat(jn_ack_buffer, jn_ACK.source);
                    strcat(jn_ack_buffer, colon_str);

                    strcat(jn_ack_buffer, jn_ACK.data);

                    int len_jn_ack_msg = strlen(jn_ack_buffer);

                    if (send(cur_fd, jn_ack_buffer, len_jn_ack_msg, 0) == -1)
                    {
                        perror("send");
                        close(sockfd);
                        exit(0);
                    }
                    printf("Sent JN_ACK to client.\n");
                } else { //TODO: specify reason for NACK
                    // Send NACK
                    // Generating NACK message
                    struct message jn_NAK = {0, 0, "", ""};

                    char nak_reason[100] = ", session doesn't exist.";

                    jn_NAK.type = 7;
                    jn_NAK.size = msg.size + strlen(nak_reason);
                    strncat(jn_NAK.source, zero, 2);
                    strncpy(jn_NAK.data, msg.data, msg.size);

                    
                    char jn_nak_buffer[4096] = {'\0'};
                    char number_buffer[4096] = {'\0'};

                    sprintf(number_buffer, "%d", jn_NAK.type);
                    strcat(jn_nak_buffer, number_buffer);
                    strcat(jn_nak_buffer, colon_str);

                    sprintf(number_buffer, "%d", jn_NAK.size);
                    strcat(jn_nak_buffer, number_buffer);
                    strcat(jn_nak_buffer, colon_str);

                    strcat(jn_nak_buffer, jn_NAK.source);
                    strcat(jn_nak_buffer, colon_str);

                    strcat(jn_nak_buffer, jn_NAK.data);
                    strcat(jn_nak_buffer, nak_reason);

                    int len_jn_nak_msg = strlen(jn_nak_buffer);

                    if (send(cur_fd, jn_nak_buffer, len_jn_nak_msg, 0) == -1)
                    {
                        perror("send");
                        close(sockfd);
                        exit(0);
                    }
                    printf("Sent JN_NAK to client.\n");
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

            if (msg.type == 11)
            { // this means it is a message packet

                printf("Recieved message from client. \n");

                int session_index = -1;

                // Finding the session the client is in
                for (int i = 0; i < NUM_USERS; i++) {
                    if (session_list[i].active && (strcmp(session_list[i].name, client_list[client_index].cur_session) == 0)) {
                        session_index = i;
                    }
                }

                if (session_list[session_index].num_users == 1) {
                    // There are no other clients in the same session
                    continue;
                }

                // Sending the message to all clients in the same session
                for (int i = 0; i < NUM_USERS; i++) {
                    if (i != client_index && client_list[i].logged_in && (strcmp(session_list[session_index].name, client_list[i].cur_session) == 0)) {
                        if (send(client_list[i].port_fd, buf, num_bytes, 0) == -1)
                        {
                            perror("send");
                            close(sockfd);
                            exit(0);
                        }
                    }
                }
                continue;
            }

            if (msg.type == 14){ // client is inviting another user to a session
                printf("Invite recieved and sending to %s.\n", msg.data);
                // Only send the invite if the reciever is not in a session and if logged in

                // Finding the invitee
                bool found = false; // TODO: tell user why invite failed?
                for (int i = 0; i < NUM_USERS; i++) {
                    if(client_list[i].logged_in && (strcmp(client_list[i].username, msg.data) == 0)) {
                        found = true;
                        // Checking if the invitee is not in a session
                        if (client_list[i].cur_session[0] == '\0') {
                            //The packet has the server in the data and the inviting client as the source
                            send_message(client_list[i].port_fd, 14, strlen(client_list[client_index].cur_session), strlen(msg.source), msg.source, client_list[client_index].cur_session);
                            printf("Sending invite\n");
                        }
                    }
                }
                continue;
            }
            if (msg.type == 15) //the client accepted the invite
            {
                printf("The client %s accepted the invite to join %s.\n", msg.source,msg.data);

                // TODO: add client to session almost like a join session (could implement this part higher to use the join session function)

                //Here the client will be waiting for a JN_ack or JN_nack message (look at the accept code in client for better idea)

                //the packet will have the session name as its data field so you can use that to add it to the session
                
                // TODO: this is just the join code copied (create a function?)

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
                    // Generating ACK message
                    struct message jn_ACK = {0, 0, "", ""};

                    jn_ACK.type = 6;
                    jn_ACK.size = msg.size;
                    strncat(jn_ACK.source, zero, 2);
                    strncpy(jn_ACK.data, msg.data, msg.size);

                    
                    char jn_ack_buffer[4096] = {'\0'};
                    char number_buffer[4096] = {'\0'};

                    sprintf(number_buffer, "%d", jn_ACK.type);
                    strcat(jn_ack_buffer, number_buffer);
                    strcat(jn_ack_buffer, colon_str);

                    sprintf(number_buffer, "%d", jn_ACK.size);
                    strcat(jn_ack_buffer, number_buffer);
                    strcat(jn_ack_buffer, colon_str);

                    strcat(jn_ack_buffer, jn_ACK.source);
                    strcat(jn_ack_buffer, colon_str);

                    strcat(jn_ack_buffer, jn_ACK.data);

                    int len_jn_ack_msg = strlen(jn_ack_buffer);

                    if (send(cur_fd, jn_ack_buffer, len_jn_ack_msg, 0) == -1)
                    {
                        perror("send");
                        close(sockfd);
                        exit(0);
                    }
                    printf("Sent JN_ACK to client.\n");
                } else { //TODO: specify reason for NACK
                    // Send NACK
                    // Generating NACK message
                    struct message jn_NAK = {0, 0, "", ""};

                    char nak_reason[100] = ", session doesn't exist.";

                    jn_NAK.type = 7;
                    jn_NAK.size = msg.size + strlen(nak_reason);
                    strncat(jn_NAK.source, zero, 2);
                    strncpy(jn_NAK.data, msg.data, msg.size);

                    
                    char jn_nak_buffer[4096] = {'\0'};
                    char number_buffer[4096] = {'\0'};

                    sprintf(number_buffer, "%d", jn_NAK.type);
                    strcat(jn_nak_buffer, number_buffer);
                    strcat(jn_nak_buffer, colon_str);

                    sprintf(number_buffer, "%d", jn_NAK.size);
                    strcat(jn_nak_buffer, number_buffer);
                    strcat(jn_nak_buffer, colon_str);

                    strcat(jn_nak_buffer, jn_NAK.source);
                    strcat(jn_nak_buffer, colon_str);

                    strcat(jn_nak_buffer, jn_NAK.data);
                    strcat(jn_nak_buffer, nak_reason);

                    int len_jn_nak_msg = strlen(jn_nak_buffer);

                    if (send(cur_fd, jn_nak_buffer, len_jn_nak_msg, 0) == -1)
                    {
                        perror("send");
                        close(sockfd);
                        exit(0);
                    }
                    printf("Sent JN_NAK to client.\n");
                }
                
                continue;
            }
            if (msg.type == 16) // the client declined the invite
            {
                printf("The client declined the invite\n");

                // TODO: should the client who invited them recieve an update on the rejection?
            }
            if (msg.type == 17) // the priv message is recieved
            {
                printf("The client %s wants to send a private message \n", msg.source);

                // TODO: Send message to the client (both client name and data are in data field)
                // if the name exists. send priv_valid_ack, if doesnt exist, send priv_invalid_ack

                char client_name[100] = {'\0'};
                int name_len = -1;

                // Separating the client name from the data
                sscanf(msg.data, "%s ", &client_name);
                name_len = strlen(client_name);

                bool found_client = false;

                // Finding the client the message is being sent to
                for (int i = 0; i < NUM_USERS; i++) {
                    if(client_list[i].logged_in && (strcmp(client_list[i].username, client_name) == 0)) {
                        found_client = true;
                        //Sending private message
                        send_message(client_list[i].port_fd, 17, msg.size - (name_len + 1), strlen(msg.source), msg.source, &msg.data[name_len + 1]);
                        //send_message(client_list[i].port_fd, 11, msg.size - (name_len + 1), strlen(msg.source), msg.source, &msg.data[name_len + 1]);
                        printf("Sending private message\n");
                    }
                }

                if(found_client) {
                    if (send(cur_fd, p_valid_buffer, len_p_valid_msg, 0) == -1) // Sending p_valid
                    {
                        perror("send");
                        close(sockfd);
                        exit(0);
                    }
                }
                else{  //the name was not found
                    if (send(cur_fd, p_invalid_buffer, len_p_invalid_msg, 0) == -1) // Sending p_invalid
                    {
                        perror("send");
                        close(sockfd);
                        exit(0);
                    }
                }
            }
        
        }
    }


    return 0;
}