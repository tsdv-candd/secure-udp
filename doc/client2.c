/*
** client2.c -- Demo program: Network Byte Order
**
** Author: atctam 
**
**
** This is the second client and it receivers an integer 
** from the server. The server must be running first, and 
** client1 be the second, and client2 must be the last.
*/


#include  <stdio.h>
#include  <unistd.h>
#include  <stdlib.h>
#include  <sys/types.h>
#include  <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>


#define PORT 49943	// the server port number
					// it must be unique within the server host machine
					// you can change this number to a port number 
					// within your allocated range

int main(int argc, char *argv[])
{
	int sd;
	typedef struct _msg
	{
		int int_value;
		char char_value;
	} structure_type;
	structure_type structure;
	struct sockaddr_in name;
	struct hostent *he;

    if (argc != 2) {
        fprintf(stderr,"usage: client2 hostname\n");
        exit(1);
    }

    // get the host info
    if ((he=gethostbyname(argv[1])) == NULL) {  
        fprintf(stderr,"gethostbyname fail\n");
        exit(1);
    }

	/* create a socket for connecting to the server */
	sd = socket(AF_INET, SOCK_STREAM, 0);

	/* establish a connection to the localhost server */
	name.sin_family = AF_INET;
    name.sin_addr = *((struct in_addr *)he->h_addr);
    name.sin_port = htons(PORT);
    memset(&(name.sin_zero), '\0', 8);

	if (connect(sd, (struct sockaddr *)&name, sizeof(name)) == -1)
		perror("client-connect");

	/* Get the data from the Server */
	recv(sd, &structure, sizeof(structure), 0);
	structure.int_value = ntohl(structure.int_value);

	printf ("\nThe data received are: %d\t%c\n", structure.int_value, structure.char_value);

	close(sd);
	return 0;
}

