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
#include <math.h>

#define _OPEN_SYS_SOCK_IPV6

struct packet
{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char *filename;
    char filedata[1000];
};


int num_digit(int num){
    int count = 0;
    do{
        num = num/10;
        count++;
    } while (num != 0);

    return count;

}


int main(int argc, char *argv[])
{

int sockfd;     // to hold the socket file descriptor
socklen_t addr_size;     //holds the size of the servers IP address 
struct addrinfo hints, *servinfo, *p;
int rv;
int numbytes;


if (argc != 3){  //just to check if both the IP address and port number were given as arguments
    fprintf(stderr, "usage: talker hostname message\n");
    exit(1);
}


memset(&hints, 0, sizeof hints);
//filling up some information about the server 

hints.ai_family = AF_INET; // set to AF_INET to use IPv4
hints.ai_socktype = SOCK_DGRAM; //for a UDP connection

if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)   // here we get the infomration based on the IP address and port number given
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

//in case we couldnt make a socket out of the IP address and port number given
if (p == NULL)
{
    fprintf(stderr, "Failed to create socket\n");
    return 2;
}

const char *msg = "ftp";  // in order to send ftp to server if a file exists  

char str[200];
char name[200];

//this is to get the filename from the terminal 
printf("Enter ftp followed by the file name: \n");
scanf("%[^\n]s \n", str);
strncpy(name, str + 4 , 100);
printf("%s\n", name);

clock_t begin; //for time

//next we can check if this file exists
if (access(name, F_OK) == 0)
{
    printf("File exists! \n");  //if file exists we send a message called ftp to server and wait for a reply
    begin = clock();
    if ((numbytes = sendto(sockfd, msg, strlen(msg), 0, p->ai_addr, p->ai_addrlen)) == -1)
    {
        perror("talker: sendto");
        exit(1);
    }
    printf("Delivered: sent %d bytes to %s\n", numbytes, argv[1]);
}
else
{   // if file doesnt exist we exit
    printf("File doesnt exist\n");
    exit(1);
}

//now we wait for server to reply
char buffer[4096];
char reply[4096] = "yes";
struct sockaddr_storage store_addr;

socklen_t length = sizeof(struct sockaddr_storage);
int rec_bytes = recvfrom(sockfd, (char *)buffer, 4096, 0, (struct sockaddr *) &store_addr, &length);
clock_t end = clock();
double round_trip_time = (double)(end - begin) / CLOCKS_PER_SEC;

printf("The round Trip time is %f seconds \n", round_trip_time);
if (rec_bytes < 0){
        perror("failed to get message , 0 bytes recieved ");
        exit(EXIT_FAILURE);
    }

buffer[length] ='\0';

//if the reply was yes, we say that the file transfer can start, else we exit

if ( strcmp( buffer, reply) == 0){  
    printf("A File Transfer Can Start\n");
}
else{
    printf("Server said no");
    exit(1);
}




// first lets open the file to read and the file to write  
FILE* file_ptr;
FILE* write_ptr;

write_ptr = fopen("rebuilt.PNG","w");
file_ptr = fopen(name, "r");

//next let us learn the size of the file
fseek(file_ptr, 0 ,SEEK_END);
long long int file_sz = ftell(file_ptr);
printf("Size of the file is %lli \n", file_sz);

//next let us learn how many packets to form
double file_size = file_sz;
int num_packets = ceil(file_size/1000.0);
printf("The number of packets is %d\n", num_packets);

//lets set the read pointer pack to the start to begin making the packets
fseek(file_ptr, 0, SEEK_SET);
struct packet packets[num_packets]; // we shall use an array of packet structs
int x = 1000 ; // max num of bytes to read

// this loop will go through each packet and fill it up with the necessary information
for (int count = 0 ; count <  num_packets ;  ++count){
  if (count == (num_packets - 1)){  // this condition makes sure we do not go above the end of file.
    x = file_size - (count) * x;
  }
  fread(packets[count].filedata, 1, x, file_ptr);
  packets[count].total_frag = num_packets;
  packets[count].frag_no = count+1;
  packets[count].filename = name;
  fwrite(packets[count].filedata, 1,x,write_ptr);
  packets[count].size = strlen(packets[count].filedata);
}

//main loop where we manipulate each packet to a string and send it and wait for acknowledgement

for (int num_p = 0 ; num_p < num_packets; ++num_p){
    char packet_buffer[4096];
    char num_buffer[4096];
    char colon_str[2] = ":";

    sprintf(num_buffer, "%d", packets[num_p].total_frag);
    strcat(packet_buffer, num_buffer);
    strcat(packet_buffer, colon_str);

    sprintf(num_buffer, "%d", packets[num_p].frag_no);
    strcat(packet_buffer, num_buffer);
    strcat(packet_buffer, colon_str);

    sprintf(num_buffer, "%d", packets[num_p].size);
    strcat(packet_buffer, num_buffer);
    strcat(packet_buffer, colon_str);

    strcat(packet_buffer, packets[num_p].filename);
    strcat(packet_buffer, colon_str);

    int pos = strlen(packet_buffer);
    int data_pos = 0;

    for (int i = pos; i < (pos + packets[num_p].size); ++i)
    {

        packet_buffer[i] = packets[num_p].filedata[data_pos];
        data_pos++;
    }


printf("The packet that we are sending is in the form: \n");
printf("%s \n", packet_buffer);

/*
if ((numbytes = sendto(sockfd, packet_buffer, pos + packets[num_p].size , 0, p->ai_addr, p->ai_addrlen)) == -1)
{
    perror("talker: sendto");
    exit(1);
}

int rec_bytes = recvfrom(sockfd, (char *)buffer, 4096, 0, (struct sockaddr *)&store_addr, &length);

// if the reply was yes, we send next packet , else we exit

if (strcmp(buffer, reply) == 0)
{
    printf("packet sent\n");
}
else
{
    printf("packet not sent");
    exit(1);
}
*/

}


//

//close the socket in the end

freeaddrinfo(servinfo);
close(sockfd);


return 0;
}