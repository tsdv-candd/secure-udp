/*
** UDP_echo_client.c -- Demo program: echo client
**
** Author: atctam
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>	/* for perror() */
#include <string.h>
/* for using socket functions */
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

	

#define PORT 49950  // the port used by target server
					// it must be unique within the server host machine
					// you can change this number to a port number 
					// within your allocated range

#define MAXDATASIZE 100 // max number of bytes we can get at once 


int main(int argc, char *argv[])
{
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in peer_addr; 	//server's address information 

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

	/* Read the manpage and slide# 27 of 03-SocketProgramming.pdf to learn how
	   to use gethostbyname() function
	*/
    if ((he=gethostbyname(argv[1])) == NULL) {  // get the host info 
        perror("gethostbyname");
        exit(1);
    }

	/* Create the socket for UDP communication */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

	/* Prepare the socket address structure of target server 
	*/
    peer_addr.sin_family = AF_INET;    // host byte order 
    peer_addr.sin_port = htons(PORT);  // short, network byte order 
    peer_addr.sin_addr = *((struct in_addr *)he->h_addr); // already in network byte order
    memset(&(peer_addr.sin_zero), '\0', 8);  // zero the rest of the struct 
	
	/* Keep this one for requesting input from user */
	printf("Enter your message:  ");
	
    /*
        1. capture the user input and store in 'buf'
        2. send 'buf' to the server
        3. wait for the echo from the server
        4. print the echo onto the screen
    */    
    while (fgets(buf, MAXDATASIZE, stdin) != NULL) {

    	buf[strlen(buf)-1]='\0';
    	if (strlen(buf) == 0) continue;
		else break;
	}
	
	/* Send the message via TCP socket */
	/* For the exercise, change this one to use sendto() for UDP */
	if (sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr_in)) == -1)
        perror("sendto");

	/* Block waiting for receiving message from server */
	/* For the exercise, you can change this to use recvfrom() */
	/* Actually, you don't need to change this if you don't care
	   who sent you the message */
    if ((numbytes=recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
        
    buf[numbytes] = '\0';

    printf("Echo received: %s\n\n",buf);

	/* close the socket */
    close(sockfd);

    return 0;
}

