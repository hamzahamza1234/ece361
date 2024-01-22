#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define _OPEN_SYS_SOCK_IPV6

int main(int argc, char *argv[])
{
char dest[4096]; //will store IP adress of server

if (inet_pton(AF_INET, argv[1], &dest) != 1){
    perror("IP address didnt convert");
    exit(EXIT_FAILURE);
}


int sockfd;
socklen_t addr_size;
struct addrinfo hints, *servinfo, *p;
int rv;
int numbytes;


if (argc != 3){
    fprintf(stderr, "usage: talker hostname message\n");
    exit(1);
}
memset(&hints, 0, sizeof hints);
hints.ai_family = AF_INET; // set to AF_INET to use IPv4
hints.ai_socktype = SOCK_DGRAM;

if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
{
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
    break;
}

if (p == NULL)
{
    fprintf(stderr, "Failed to create socket\n");
    return 2;
}

const char *msg = "ftp";

char str[200];
char name[200];

//this is to get the filename from the terminal 
printf("Enter ftp followed by the file name: \n");
scanf("%[^\n]s \n", str);
strncpy(name, str + 4 , 100);
printf("%s\n", name);

//next we can check if this file exists
if (access(name, F_OK) == 0)
{
    printf("File exists! \n");  //if file exists we send a message called ftp to server and wait for a reply
    if ((numbytes = sendto(sockfd, msg, strlen(msg), 0, p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }
    printf("Delivered: sent %d bytes to %s\n", numbytes, argv[1]);
}
else
{   //if file doesnt exist we exit
    printf("File doesnt exist\n");
    exit(1);
}

//now we wait for server to reply

char buffer[4096];
char reply[4096] = "yes";
struct sockaddr_storage store_addr;

int length = sizeof(struct sockaddr_storage);
int rec_bytes = recvfrom(sockfd, (char *)buffer, 4096, 0, (struct sockaddr *) &store_addr, &length);

if (rec_bytes < 0){
        perror("failed to get message , 0 bytes recieved ");
        exit(EXIT_FAILURE);
    }

buffer[length] ='\0';

if ( strcmp( buffer, reply) == 0){
    printf("A File Transfer Can Start\n");
}
else{
    printf("Server said no");
    exit(1);
}




freeaddrinfo(servinfo);
close(sockfd);


return 0;
}