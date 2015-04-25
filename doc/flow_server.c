/*
** flow_server.c -- Demo program: Network Byte Order
**
** Author: atctam 
**
**
** This is the server that receives an integer and a character  
** from clientl.c from one socket. The server will send the  
** updated integer and character to client2.c via another socket.
*/

/* ------------------ flow_server.c --------------------- */

/* This is the server that reads a character from clientl.c from the socket.
The server will write the character to the socket and send it to client2.c */

#include  <stdio.h>
#include  <unistd.h>
#include  <sys/types.h>
#include  <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>



#define PORT 49943	// the server port number
					// it must be unique within the server host machine
					// you can change this number to a port number 
					// within your allocated range


int main()
{
	int sd, c1, c2;
	socklen_t lin;
	typedef struct _msg
	{
		int int_value;
		char char_value;
	} structure_type;

	structure_type structure;
	struct sockaddr_in name, namein; /* create socket for requests */

	sd = socket(AF_INET, SOCK_STREAM, 0);

	name.sin_family = AF_INET;
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    name.sin_port = htons(PORT);
    memset(&(name.sin_zero), '\0', 8);
    
	/* bind it to the created socket, and set max queue length */
	if (bind(sd, (struct sockaddr *)&name, sizeof(name)) != 0)
		perror("server-bind");

	listen(sd,5);

	/* accept two connections */
	/* ASSUMPTION: client1.c should establish the connection to server first
	               client2.c should connect afterwards
	*/
	lin = sizeof(namein);
	if ((c1 = accept(sd, (struct sockaddr *)&namein, &lin)) == -1)
		perror("accept-error");
	if ((c2 = accept(sd, (struct sockaddr *)&namein, &lin)) == -1)
		perror("accept-error");

	/* close sd - use only c1 and c2*/
	close(sd);

	/* wait for message from client1 first */
	recv(c1, &structure, sizeof(structure), 0);
	structure.int_value = ntohl(structure.int_value);

	printf ("\nThe data received are: %d\t%c\n", structure.int_value, structure.char_value);

	/* increment the value stored in structure */
	structure.int_value++;
	structure.char_value++;

	printf ("\nThe data stored in structure become: %d\t%c\n", structure.int_value, structure.char_value);
	structure.int_value = htonl(structure.int_value);

	/* send to client2 */
	send(c2, &structure, sizeof(structure), 0);
	
	/* main server closes ns */
	close(c1);
	close(c2);
	
	return 0;
}
