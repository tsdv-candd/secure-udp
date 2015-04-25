/**************************************************************
rdt-part2.h
Student name:
Student No. :
Date and version:
Development platform:
Development language:
Compilation:
	Can be compiled with
*****************************************************************/

#ifndef RDT2_H
#define RDT2_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>

#define PAYLOAD 	1000		//size of data payload of the RDT layer
#define DATA_LENGTH 	2		//length of data in packet
#define CHECKSUM_LENGTH	2		//length of checksum
#define DATA_TYPE	1		    //data type
#define TIMEOUT     5000		//50 milliseconds
#define TWAIT 		10*TIMEOUT	//Each peer keeps an eye on the receiving  
#define RETRY_TIME	5

//end for TWAIT time units before closing
//For retransmission of missing last ACK

//----- Type defines ----------------------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

typedef struct rdt_packet_ {
    u16b_t 	len;
    u16b_t 	checksum;
    u8b_t 	type;
    u8b_t    data[PAYLOAD];
} rdt_packet;

enum PACKET_TYPE {DATA, ACK, NACK};

extern float LOSS_RATE, ERR_RATE;

struct sockaddr_in my_addr, peer_addr;
fd_set nfds;
fd_set readfds;
struct timeval timer;

/* this function is for simulating packet loss or corruption in an unreliable channel */
/***
Assume we have registered the target peer address with the UDP socket by the connect()
function, udt_send() uses send() function (instead of sendto() function) to send
a UDP datagram.
***/
int udt_send(int fd, void * pkt, int pktLen, unsigned int flags) {
    double randomNum = 0.0;

    /* simulate packet loss */
    //randomly generate a number between 0 and 1
    randomNum = (double)rand() / RAND_MAX;
    if (randomNum < LOSS_RATE) {
        //simulate packet loss of unreliable send
        printf("WARNING: udt_send: Packet lost in unreliable layer!!!!!!\n");
        return pktLen;
    }

    /* simulate packet corruption */
    //randomly generate a number between 0 and 1
    randomNum = (double)rand() / RAND_MAX;
    if (randomNum < ERR_RATE) {
        //clone the packet
        u8b_t errmsg[pktLen];
        memcpy(errmsg, pkt, pktLen);
        //change a char of the packet
        int position = rand() % pktLen;
        if (errmsg[position] > 1) errmsg[position] -= 2;
        else errmsg[position] = 254;
        printf("WARNING: udt_send: Packet corrupted in unreliable layer!!!!!!\n");
        return send(fd, errmsg, pktLen, 0);
    } else 	// transmit original packet
        return send(fd, pkt, pktLen, 0);
}

/* this function is for calculating the 16-bit checksum of a message */
/***
Source: UNIX Network Programming, Vol 1 (by W.R. Stevens et. al)
***/

u16b_t checksum(void *msg, u16b_t bytecount)
{
    u32b_t sum = 0x0000;
    u16b_t * addr = (u16b_t *)msg;
    u32b_t word = 0x00;

    // add 16-bit by 16-bit
    while(bytecount > 1)
    {
        sum += *addr++;
        bytecount -= 2;
    }

    // Add left-over byte, if any
    if (bytecount > 0) {
        *(u8b_t *)(&word) = *(u8b_t *)addr;
        sum += word;
    }

    // Fold 32-bit sum to 16 bits
    while (sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    word = ~sum & 0xFF;
    return word;
}
#if 0
u16b_t checksum(u8b_t *msg, u16b_t bytecount)
{
    u32b_t sum = 0;
    u16b_t * addr = (u16b_t *)msg;
    u16b_t word = 0;

    // add 16-bit by 16-bit
    while(bytecount > 1)
    {
        sum += *addr++;
        bytecount -= 2;
    }

    // Add left-over byte, if any
    if (bytecount > 0) {
        *(u8b_t *)(&word) = *(u8b_t *)addr;
        sum += word;
    }

    // Fold 32-bit sum to 16 bits
    while (sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    word = ~sum;

    return word;
}
#endif /*If 0*/

//----- Type defines ----------------------------------------------------------

// define your data structures and global variables in here




int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, void * msg, int length);
int rdt_recv(int fd, void * msg, int length);
int rdt_close(int fd);
u16b_t make_pkt(void * msg, u16b_t length, u8b_t type, void * outmsg);

/* Application process calls this function to create the RDT socket.
   return	-> the socket descriptor on success, -1 on error
*/
int rdt_socket() {
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        return -1;
    }
    else return sockfd;
}

/* Application process calls this function to specify the IP address
   and port number used by itself and assigns them to the RDT socket.
   return	-> 0 on success, -1 on error
*/
int rdt_bind(int fd, u16b_t port) {
    my_addr.sin_family = AF_INET;       // host byte order
    my_addr.sin_port = htons(port);     // short, network byte order
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY); // automatically fill with my IP address
    memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

    if (bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        return -1;
    }
    else return 0;

}

/* Application process calls this function to specify the IP address
   and port number used by remote process and associates them to the
   RDT socket.
   return	-> 0 on success, -1 on error
*/
int rdt_target(int fd, char * peer_name, u16b_t peer_port) {
    struct hostent *tg;
    //struct sockaddr_in peer_addr;
    if ((tg=gethostbyname(peer_name)) == NULL) {  // get the host info
        perror("gethostbyname");
        return -1;
    }
    peer_addr.sin_family = AF_INET;    // host byte order
    peer_addr.sin_port = htons(peer_port);  // short, network byte order
    peer_addr.sin_addr = *((struct in_addr *)tg->h_addr); // already in network byte order
    memset(&(peer_addr.sin_zero), '\0', 8);  // zero the rest of the struct


    if (connect(fd, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr)) == -1)
    {   perror("connect");
        return(-1);
    }
    else return 0;
}

/* Application process calls this function to transmit a message to
   target (rdt_target) remote process through RDT socket.
   msg		-> pointer to the application's send buffer
   length	-> length of application message
   return	-> size of data sent on success, -1 on error
*/
int rdt_send(int fd, void * msg, int length) {
    //implement the Stop-and-Wait ARQ (rdt3.0) logic
    //must use the udt_send() function to send data via the unreliable layer
    struct timeval tv;
    socklen_t size;
    fd_set rfds;
    u8b_t sendpkt[DATA_LENGTH + DATA_TYPE + CHECKSUM_LENGTH + length];
    u8b_t recvpkt[DATA_LENGTH + DATA_TYPE + CHECKSUM_LENGTH + PAYLOAD];
    int len = CHECKSUM_LENGTH + DATA_LENGTH + DATA_TYPE + PAYLOAD;
    int sendlen = 0;
    int rlen = 0;


    //make packet data
    memset(sendpkt, 0, DATA_LENGTH + DATA_TYPE + CHECKSUM_LENGTH + length);
    sendlen = make_pkt((void*)msg, length, DATA, (void*)sendpkt);
    printf("Send leng = [%d] intput leng[%d]\n",sendlen,length);
    //set timeout for recv function
    tv.tv_sec = 0;
    tv.tv_usec = TIMEOUT;  // Not init'ing this can cause strange errors
    FD_ZERO (&rfds);
    FD_SET (fd, &rfds);

    for (;;) {
        // error occour
        printf("Start send data ..1\n");
        //if (select(fd+1, &rfds, NULL, NULL, &tv) == 0 ) {
        //    Printf("TIME OUT SEND");
        //    return(-1);
        //}
        //printf("Start send data ..2\n");
        //send data
        if(udt_send(fd, sendpkt, sendlen, 0) == -1) {
            perror("send");
            continue;
            //return(-1);
        } else {
            for (;;) {
                //no error occour
                if (select(fd+1, &rfds, NULL, NULL, &tv) <= 0) {
                    perror("send");
                    printf("TIMEOUT of waiting ACK\n");
                    return(-1);
                }

                //receive data
                if ((rlen=recvfrom(fd, recvpkt, len, 0, (struct sockaddr*) &peer_addr, &size))==-1) {
                    return -1;
                }
                rdt_packet tmp_pckt;
                tmp_pckt.checksum = (recvpkt[0] << 8) | (recvpkt[1] & 0xFF);
                printf("Send Checksum [%d]\n", tmp_pckt.checksum);
                tmp_pckt.len = (recvpkt[CHECKSUM_LENGTH + 0] << 8) | (recvpkt[CHECKSUM_LENGTH + 1] & 0xFF);
                tmp_pckt.type = recvpkt[CHECKSUM_LENGTH + DATA_LENGTH];
                //memcpy(tmp_pckt.data, &recvpkt[CHECKSUM_LENGTH + DATA_LENGTH + DATA_TYPE], tmp_pckt.len);
                if (tmp_pckt.type == ACK)
                {
                    //would we do if reply message is not ACK type?
                    printf("Send Get ACK [%d]\n", tmp_pckt.len);
                    return sendlen;
                } else {
                    printf("Continue Wait ACK [%d]\n", tmp_pckt.len);
                    continue;
                }
            }
        }
    }
}

/* Application process calls this function to wait for a message
   from the remote process; the caller will be blocked waiting for
   the arrival of the message.
   msg		-> pointer to the receiving buffer
   length	-> length of receiving buffer
   return	-> size of data received on success, -1 on error
*/
int rdt_recv(int fd, void * msg, int length) {
    //implement the Stop-and-Wait ARQ (rdt3.0) logic
    int len = CHECKSUM_LENGTH + DATA_LENGTH + DATA_TYPE + PAYLOAD;
    u8b_t buf[CHECKSUM_LENGTH + DATA_LENGTH + DATA_TYPE + PAYLOAD];
    int relen = 0;
    u16b_t tmpchecksum = 0;
    socklen_t size = sizeof (struct sockaddr_in);

    for (;;) {
        if ((relen = recvfrom(fd, buf, len, 0, (struct sockaddr*) &peer_addr, &size)) == -1 ) {
            printf("RECV: Failed to recieve\n");
            perror("recv");
            return -1;
        }

        rdt_packet tmp_pckt;
        tmp_pckt.checksum = (buf[0] << 8) | (buf[1] & 0xFF);
        tmp_pckt.len = (buf[CHECKSUM_LENGTH + 0] << 8) | (buf[CHECKSUM_LENGTH + 1] & 0xFF);
        tmp_pckt.type = buf[CHECKSUM_LENGTH + DATA_LENGTH];

        memcpy(tmp_pckt.data, &buf[CHECKSUM_LENGTH + DATA_LENGTH + DATA_TYPE], tmp_pckt.len);
        printf("Recived leng [%d] But package leng [%d]\n",relen, tmp_pckt.len);
        printf("RECV Recived check sum : [%d] \n",tmp_pckt.checksum);
        //calculate checksum
        tmpchecksum = checksum(tmp_pckt.data, tmp_pckt.len);
        printf("RECV Calculate check sum : [%d] \n",tmpchecksum);
        if(tmpchecksum == tmp_pckt.checksum) {
            //If RECV DATA
            if(tmp_pckt.type == DATA) {
                printf("Recv Get DATA ...\n");
                //Make ACK package and send back
                u8b_t sndpkt[DATA_LENGTH + DATA_TYPE + CHECKSUM_LENGTH + PAYLOAD];
                make_pkt(NULL, 0, ACK, sndpkt);
                udt_send(fd, sndpkt, (DATA_LENGTH + DATA_TYPE + CHECKSUM_LENGTH + PAYLOAD), 0);

                //send data to upper layer
                memcpy(msg, tmp_pckt.data, tmp_pckt.len);
                return tmp_pckt.len;
            } else if  (tmp_pckt.type == ACK) { //If RECV ACK
                printf("Recv Get ACK ...\n");
                //Make sure no send data to upper layer
                memset(msg, 0, length);
                return 0;
            } else { //NOT DATA OR ACK
                printf("Recv Get NOT ACK OR DATA ...\n");
                //Make sure no send data to upper layer
                memset(msg, 0, length);
                return 0;
            }
        } else {
            printf("GET DATA BUT WRONG CHECKSUM...\n");
            //The checksum mitmatch, Send NACK
            u8b_t sndpkt[DATA_LENGTH + DATA_TYPE + CHECKSUM_LENGTH + PAYLOAD];
            make_pkt(NULL, 0, NACK, sndpkt);
            udt_send(fd, sndpkt, (DATA_LENGTH + DATA_TYPE + CHECKSUM_LENGTH + PAYLOAD), 0);
            //Make sure no send data to upper layer
            memset(msg, 0, length);
            return 0;
        }
    }
}

/* Application process calls this function to close the RDT socket.
*/
int rdt_close(int fd) {
    //implement the Stop-and-Wait ARQ (rdt3.0) logic
    int relen = 0;
    int status;
    socklen_t size;
    FD_ZERO (&nfds);
    FD_ZERO (&readfds);
    FD_SET(fd, &nfds);
    u16b_t tmpchecksum = 0;
    int len = CHECKSUM_LENGTH + DATA_LENGTH + DATA_TYPE + PAYLOAD;
    u8b_t buf[CHECKSUM_LENGTH + DATA_LENGTH + DATA_TYPE + PAYLOAD];

    timer.tv_sec=0;
    timer.tv_usec= TWAIT;

    //Close socket
    for(;;) {

        readfds = nfds;

        if ((status=select(fd+1, &readfds, NULL, NULL, &timer))==-1) {
            return -1;
        } else if( status==0 ) {
            close(fd);
            return 0;
        } else {
            if ((relen=recvfrom(fd, buf, len, 0, (struct sockaddr*) &peer_addr, &size))==-1) {
                perror("recv");
                return(-1);
            }

            rdt_packet tmp_pckt;
            tmp_pckt.checksum = (buf[0] << 8) | (buf[1] & 0xFF);
            tmp_pckt.len = (buf[CHECKSUM_LENGTH + 0] << 8) | (buf[CHECKSUM_LENGTH + 1] & 0xFF);
            tmp_pckt.type = buf[CHECKSUM_LENGTH + DATA_LENGTH];
            memcpy(tmp_pckt.data, &buf[CHECKSUM_LENGTH + DATA_LENGTH + DATA_TYPE], tmp_pckt.len);

            printf("Recived leng [%d] But package leng [%d]\n",relen, tmp_pckt.len);
            printf("RECV Recived check sum : [%d] \n",tmp_pckt.checksum);
            //calculate checksum
            tmpchecksum = checksum(tmp_pckt.data, tmp_pckt.len);
            printf("RECV Calculate check sum : [%d] \n",tmpchecksum);
            if (tmpchecksum == tmp_pckt.checksum) {
                u8b_t sndpkt[DATA_LENGTH + DATA_TYPE + CHECKSUM_LENGTH + PAYLOAD];
                make_pkt(NULL, 0, ACK, sndpkt);
                udt_send(fd, sndpkt, (DATA_LENGTH + DATA_TYPE + CHECKSUM_LENGTH + PAYLOAD), 0);
                perror("not closing\n");
            }
        }
    }

    return 0;
}

/* Make packet data from message and length
*/
u16b_t make_pkt(void * msg, u16b_t length, u8b_t type, void * outmsg) {
    u16b_t totallen = 0;
    u16b_t chksumno = 0;
    u8b_t *tmp = (u8b_t *)outmsg;

    totallen = CHECKSUM_LENGTH + DATA_LENGTH + DATA_TYPE + length;

    chksumno = checksum(msg, length);

    printf("Make Package: CHECKSUM [%d]...\n", chksumno);

    //add
    tmp[0] = (chksumno >> 8) & 0xFF;
    tmp[1] = (chksumno) & 0xFF;

    //add length
    tmp[CHECKSUM_LENGTH + 0] = (length >> 8) & 0xFF;
    tmp[CHECKSUM_LENGTH + 1] = (length) & 0xFF;

    //add type
    tmp[CHECKSUM_LENGTH + DATA_LENGTH] = type;

    //add data
    if (msg != NULL) {
        memcpy(&tmp[CHECKSUM_LENGTH + DATA_LENGTH + DATA_TYPE], msg, length);
    }

    return totallen;
}

int rdt_select(int fdes , int w_sec  )
{
    fd_set          fdset ;
    int             numfds, status ;
    struct timeval  tvp ;
#ifdef DEBUG
#endif
    numfds = 0 ;
    FD_ZERO(&fdset) ;
    numfds = 10;
    FD_SET(fdes,&fdset) ;

    if ( w_sec != -1 ) {
        tvp.tv_sec  = w_sec ;
        tvp.tv_usec = 0 ;
        status = select(numfds,(fd_set*)&fdset,0,0,&tvp) ;
    } else {
        status = select(numfds,(fd_set*)&fdset,0,0,0 );
    }
    if ( status > 0 ) {
        return ( 1);
    } else if ( status == 0 ) {
        return ( 0 );
    } else {
        return ( -1 );
    }
}

#endif
