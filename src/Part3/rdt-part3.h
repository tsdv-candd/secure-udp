/**************************************************************
rdt-part3.h
Student name:
Student No. :
Date and version:
Development platform:
Development language:
Compilation:
	Can be compiled with
*****************************************************************/

#ifndef RDT3_H
#define RDT3_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

#define PAYLOAD 	1000	//size of data payload of the RDT layer
#define DATA_LENGTH 	2	//length of data in packet
#define CHECKSUM_LENGTH	2	//length of checksum
#define DTYPE_LENGTH	1	//data type
#define SEQNO_LENGTH   	1 	// Sequence number length.
#define HEADER 		   	(CHECKSUM_LENGTH + DATA_LENGTH + DTYPE_LENGTH + SEQNO_LENGTH)
#define TIMEOUT 50000		//50 milliseconds
#define TWAIT 10*TIMEOUT	//Each peer keeps an eye on the receiving  
//end for TWAIT time units before closing
//For retransmission of missing last ACK
#define RETRY_TIME	5

#define W 5					//For Extended S&W - define pipeline window size


//----- Type defines ----------------------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word

typedef struct rdt_packet_ {
    u16b_t 	len;
    u16b_t 	checksum;
    u8b_t 	type;
    u8b_t	seqno;
    u8b_t   data[PAYLOAD];
} rdt_packet;

enum PACKET_TYPE {DATA = 0x00, ACK, NACK};

extern float LOSS_RATE, ERR_RATE;

struct sockaddr_in my_addr, peer_addr;
fd_set nfds;
fd_set readfds;
struct timeval timer;
int sockfd;

int lastapk = 0;
int lastpktlen = 0;
int dummy[9];
u8b_t nextpkt=0;

u8b_t seqdata=0;
int lastpktack=0;

char empty[1]= {'0'};
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
    } else 	{// transmit original packet
        printf("Transmit data\n");
        return send(fd, pkt, pktLen, 0);
    }
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

//----- Type defines ----------------------------------------------------------

// define your data structures and global variables in here

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, void * msg, int length);
int rdt_recv(int fd, void * msg, int length);
int rdt_close(int fd);
u16b_t make_pkt(void * msg, u8b_t seqno, u16b_t length, PACKET_TYPE type, void * outmsg, int j);
void analyze_package_header(u8b_t *inbuf, u16b_t &heachecksum, u16b_t &calchecksum, u16b_t &len, u8b_t &type, u8b_t &seqno);

// To update the seq#
void updateSeqno(int* seqno) {
    if (*seqno <5) {
        *seqno=*seqno+1;
    } else *seqno =0;
}

int count_pkt(int length, int *lastpktlen) {
    *lastpktlen =length %PAYLOAD;
    if (*lastpktlen==0) {
        *lastpktlen =PAYLOAD;
        return (length/PAYLOAD);
    } else {
        return (length/PAYLOAD)+1;
    }
}

/* Application process calls this function to create the RDT socket.
   return	-> the socket descriptor on success, -1 on error
*/
int rdt_socket() {
//same as part 1
    //int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        //
        perror("socket");
        return -1;
    }
    return sockfd;
}

/* Application process calls this function to specify the IP address
   and port number used by itself and assigns them to the RDT socket.
   return	-> 0 on success, -1 on error
*/
int rdt_bind(int fd, u16b_t port) {
//same as part 1
    my_addr.sin_family = AF_INET;       // host byte order
    my_addr.sin_port = htons(port);     // short, network byte order
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY); // automatically fill with my IP address
    memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct

    if (bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        //perror("bind");
        return -1;
    }
    return 0;
}

/* Application process calls this function to specify the IP address
   and port number used by remote process and associates them to the
   RDT socket.
   return	-> 0 on success, -1 on error
*/
int rdt_target(int fd, char * peer_name, u16b_t peer_port) {
//same as part 1
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
    {   //perror("connect");
        return(-1);
    }
    else return 0;
}

/* Application process calls this function to transmit a message to
   target (rdt_target) remote process through RDT socket; this call will
   not return until the whole message has been successfully transmitted
   or when encountered errors.
   msg		-> pointer to the application's send buffer
   length	-> length of application message
   return	-> size of data sent on success, -1 on error
*/
int rdt_send(int fd, void * msg, int length) {
//implement the Extended Stop-and-Wait ARQ logic

//must use the udt_send() function to send data via the unreliable layer
    struct timeval timer1;
    socklen_t size;
    int status;
    //int base_len = CHECKSUM_LENGTH + DATA_LENGTH + DTYPE_LENGTH + SEQNO_LENGTH;
    u8b_t recvpkt[ HEADER + PAYLOAD];
    int sendlen = 0;
    int rlen = 0;
    u8b_t seqbegin;
    u8b_t seqend;
    u16b_t heachecksum, calchecksum;
    u16b_t len;
    u8b_t type;
    u8b_t seqno;
    FD_ZERO (&nfds);
    FD_ZERO (&readfds);
    FD_SET(fd, &nfds);
    //u8b_t outbuf;

    //Calculate Total number of package
    int N = count_pkt(length, &lastpktlen);
    printf("Total package [%d], lastpckagelen = [%d]\n", N, lastpktlen);
    u8b_t sendpkt[N-1][HEADER + PAYLOAD];
    u8b_t lastsendpkt[HEADER + lastpktlen];

    seqbegin=seqdata;

    //Send all package
    for (int i=0; i<N; i++) {
        if (i!= N-1) {
            //Send one by one except last package.
            printf("SEND: [%d]\n", i);
            sendlen = make_pkt((void*)msg, seqdata, PAYLOAD, DATA, (void*)sendpkt[i], i);
            udt_send(fd, sendpkt[i], sendlen, 0);
            printf("send 1000 pkt\n");
        } else {
            //last package.
            printf("SEND: last package no[%d]length [%d]\n", i, lastpktlen);
            sendlen = make_pkt((void*)msg, seqdata, lastpktlen, DATA, (void*)lastsendpkt, i);
            udt_send(fd, lastsendpkt, sendlen, 0);
            printf("0 SEND: last package length\n");
        }
        printf("1 SEND: last package length\n");
        if (seqdata<127) { //out of range of signed char problem
            seqdata++;
        } else {
            seqdata=0;
        }
    }

    seqend=seqdata-1;
    lastpktack =0;

    printf("2 SEND: Check ACK \n");
    int i = 0;
    for (;;) {
        printf("3 SEND: Check ACK\n");
        readfds = nfds;
        timer1.tv_sec=0;
        timer1.tv_usec= TIMEOUT;

        //Start timer
        printf("END: Start Timer ..1\n");
        if ((status = select(fd+1, &readfds, NULL, NULL, &timer1)) == -1 ) {
            return(-1);	//Other error
        } else if (status == 0) { //Time out
            printf("SEND: TIME OUT REND ALL SEND [%d]\n", i++);
            //Retransmit from all unACKed packets
            for (int i= (int)lastpktack; i<N; i++) {
                printf("SEND : Resend all package because of time out\n");
                if (i!=N-1) {
                    printf("SEND: 0 TIME OUT REND ALL SEND\n");
                    udt_send(fd, sendpkt[i], HEADER + PAYLOAD, 0);
                } else {
                    printf("SEND: LAST PACKAGE TIME OUT REND ALL SEND\n");
                    udt_send(fd, lastsendpkt, HEADER + lastpktlen, 0);
                }
            }
        } else {
            printf("SEND : Wait ACK data from socket\n");
            if ((rlen=recvfrom(fd, recvpkt, (HEADER + PAYLOAD), 0, (struct sockaddr*) &peer_addr, &size))==-1) {
                return -1;
            }
            //rdt_packet tmp_pckt;
            analyze_package_header(recvpkt, heachecksum, calchecksum, len, type, seqno);
            printf("SEND: headchecksum = [%d], calchecksum = [%d], len [%d], type [%d], seqno[%d]\n", \
                   heachecksum, calchecksum, len, type, seqno);
            if(heachecksum == calchecksum && type == ACK) {
                printf("SEND : Get ACK [%d]\n", len);
                if (seqno == seqend) {
                    printf("SEND : tmp_pckt.seqno [%d]\n", seqend);
                    return length;
                }
                if(seqno - seqbegin <= N && seqend - seqno <= N) {
                    printf("SEND : Has not success!!! \n");
                    lastpktack = seqno - seqbegin;
                } else {
                    printf("SEND : lastpktack [%d]\n", lastpktack);
                    for(int i= (int)lastpktack; i < N; i++) {
                        if (i != N-1) {
                            printf("SEND : sendpkt \n");
                            udt_send(fd, sendpkt[i], HEADER + PAYLOAD, 0);
                        } else {
                            printf("SEND : send lastsendpkt \n");
                            udt_send(fd, lastsendpkt, HEADER + lastpktlen, 0);
                        }
                    }
                }
            }
            // Received noncorrupted data pkt
            else if (heachecksum == calchecksum && type == DATA) {
                printf("SEND : Revive DATA from socket \n");
                return length;
            }
            //received corrupted pkt
            else if (heachecksum != calchecksum) {
                //Rend from wrong check sum packages
                printf("SEND : Wrong checksum and resend\n");
                for (int i= (int)lastpktack; i<N; i++) {
                    if (i!=N-1) {
                        printf("SEND : Wrong checksum Resend seqno[%d] lastpkg [%d] \n", seqno,lastapk);
                        udt_send(fd, sendpkt[i], HEADER + PAYLOAD, 0);
                    } else {
                        printf("SEND : Wrong checksum Resend seqno[%d] lastpkg [%d] \n", seqno,lastapk);
                        udt_send(fd, lastsendpkt, HEADER + lastpktlen, 0);
                    }
                }
            }
        }
    }
}

/* Application process calls this function to wait for a message of any
   length from the remote process; the caller will be blocked waiting for
   the arrival of the message.
   msg		-> pointer to the receiving buffer
   length	-> length of receiving buffer
   return	-> size of data received on success, -1 on error
*/
int rdt_recv(int fd, void * msg, int length) {
//implement the Extended Stop-and-Wait ARQ logic
    socklen_t size = sizeof (struct sockaddr_in);
    int relen = 0;
    u8b_t buf[HEADER + PAYLOAD];
    int len = HEADER + PAYLOAD;
    u8b_t newack[HEADER];
    //u16b_t tmpchecksum = 0;
    int sendlen = 0;
    u16b_t heachecksum, calchecksum;
    u16b_t datalen=0;
    u8b_t type=0;
    u8b_t seqno = 0;
    int i = 0;
    //nextpkt = 0;
    for(;;) {
        memset(buf, 0, len);
        printf("RECV : WAIT DATA From socket [%d]\n",i++);
        if ((relen = recvfrom(fd, buf, len, 0, (struct sockaddr*) &peer_addr, &size)) == -1 ) {
            //perror("recv");
            return -1;
        }
        printf("RECV : FOUND DATA From socket \n");
        //Analyse RECV data
        analyze_package_header(buf, heachecksum, calchecksum, datalen, type, seqno);
        printf("SEND: headchecksum = [%d], calchecksum = [%d], len [%d], type [%d], seqno[%d]\n", \
               heachecksum, calchecksum, datalen, type, seqno);
        if(type == DATA) {
            printf("RECV :check1 nextpkt=%u\n", nextpkt);
            if(heachecksum == calchecksum && nextpkt == seqno) {
                printf("RECV : DATA check sum correct : [%d] \n", calchecksum);
                sendlen = make_pkt(empty, nextpkt, HEADER, ACK, (void*)newack,0);
                //make_pkt(1, nextpkt, empty, newack, 5, 0);
                //Send ACK back
                udt_send(fd, newack, sendlen, 0);
                memcpy(msg, &buf[HEADER], datalen);

                if (nextpkt<127) { //out of range of signed char problem
                    nextpkt++;
                } else {
                    nextpkt=0;
                }
                printf("RECV :check1 nextpkt=%u\n", nextpkt);
                return (datalen);
            } else {
                printf("RECV: Wrong recv sum [%d] vs calum [%d] OR \n seqno [%d] vs nextpkt [%d]\n", \
                       heachecksum, calchecksum, seqno, nextpkt);
                sendlen = make_pkt(empty, nextpkt-1, HEADER, ACK, newack, 0);
                udt_send(fd, newack, sendlen, 0);
                //return 0;
            }
        } else if (type == ACK || type == NACK) { //Do no thing if packet is ACK
            printf("RECV: ACK package\n");
            //continue;
            //return 0;
        }
    }
}

/* Application process calls this function to close the RDT socket.
*/
int rdt_close(int fd) {
//implement the Extended Stop-and-Wait ARQ logic
    int relen = 0;
    int status;
    socklen_t size;
    FD_ZERO (&nfds);
    FD_ZERO (&readfds);
    FD_SET(fd, &nfds);
    u16b_t heachecksum, calchecksum;
    u16b_t datalen;
    u8b_t type;
    u8b_t seqno;

    timer.tv_sec=0;
    timer.tv_usec= TWAIT;

    //u16b_t tmpchecksum = 0;
    u8b_t buf[HEADER + PAYLOAD];
    int len = HEADER + PAYLOAD;

    //Close socket
    for(;;) {

        readfds = nfds;

        if ((status=select(fd+1, &readfds, NULL, NULL, &timer))==-1) {
            //printf("CLOSE: Time out\n");
            return -1;
        } else if( status==0 ) {
            printf("CLOSE: close SOCKET\n");
            close(fd);
            return 0;
        } else {
            printf("CLOSE: start receive from socket [%d]\n", len);
            memset(buf, 0, len);
            if ((relen=recvfrom(fd, buf, len, 0, (struct sockaddr*) &peer_addr, &size))==-1) {
                //printf("CLOSE: Could not receive from socket\n");
                //perror("recv");
                return(-1);
            }
            analyze_package_header(buf, heachecksum, calchecksum, datalen, type, seqno);
            printf("SEND: headchecksum = [%d], calchecksum = [%d], len [%d], type [%d], seqno[%d]\n", \
                   heachecksum, calchecksum, datalen, type, seqno);
            printf("CLOSE : recv leng [%d] But package leng [%d]\n",relen, datalen);
            printf("CLOSE : recv check sum : [%d] \n", calchecksum);
            //calculate checksum
            if (type == DATA && seqno == nextpkt) {
                if(heachecksum == calchecksum) {
                    u8b_t pktsend [HEADER];
                    int sendlen = 0;
                    ///make_pkt(1, pktrecv[1], empty, pktsend, 5, 0);
                    printf("CLOSE: Not close and resend ACK, [seqno] [%d]\n",seqno);
                    sendlen = make_pkt(empty, seqno, HEADER, ACK, (void*)pktsend,0);
                    udt_send(fd, pktsend, sendlen, 0);
                    //perror("not closing\n");
                }
            }
        }
    }
    printf("CLOSE: DONE\n");
    return 0;
}

/* Make packet data from message and length
 msg 	-> input message
 seqno 	-> seq#
 length -> length of the input message
 type 	-> 0 -> DATA, 1 ACK, 2, NACK.
 outmsg -> output message. (must have length + header)
*/
u16b_t make_pkt(void *msg, u8b_t seqno, u16b_t length, PACKET_TYPE type, void *outmsg, int j) {
    u16b_t totallen = 0;
    u16b_t chksumno = 0;
    u8b_t *tmp = (u8b_t *)outmsg;
    u8b_t *tmp1 = (u8b_t *)msg;

    if(outmsg == NULL) {
        //Could not create package with NULL output.
        return 0;
    }
    printf("Make_PKT: TYPE [%d] lengt: [%d], seqno [%d]\n",type, length, seqno);

    //add length
    tmp[CHECKSUM_LENGTH + 0] = (length >> 8) & 0xFF;
    tmp[CHECKSUM_LENGTH + 1] = (length) & 0xFF;

    //add type
    tmp[CHECKSUM_LENGTH + DATA_LENGTH] = type;
    tmp[CHECKSUM_LENGTH + DATA_LENGTH + DTYPE_LENGTH] = seqno;

    //Calculate checksum then add data
    if (type == DATA ) { //For data packet, checks the packet type, seq# and data.
        if(msg != NULL ) {
            totallen = HEADER + length;
            //memcpy(&tmp[CHECKSUM_LENGTH + DATA_LENGTH + DTYPE_LENGTH + SEQNO_LENGTH], msg, length);
            for (int i=0; i<length; i++) {
                tmp[HEADER +i] = tmp1[i +j*PAYLOAD];
            }
            //printf("!!MAKE_PKT Start making DATA with PAYLOAD = [%d], Start Pos [%d], start Data [%d]\n", length, CHECKSUM_LENGTH + DATA_LENGTH,tmp[CHECKSUM_LENGTH + DATA_LENGTH ]);
            chksumno = checksum(&tmp[CHECKSUM_LENGTH + DATA_LENGTH ], DTYPE_LENGTH + SEQNO_LENGTH + length);
            //add checksum
            tmp[0] = (chksumno >> 8) & 0xFF;
            tmp[1] = (chksumno) & 0xFF;
            return totallen;
        } else {
            return 0;
        }
    } else if ((type == ACK ) | (type = NACK )) {
        printf("MAKE_PKT : ACK \n");
        //Calculate check sum for "packet type"" and "seqno".
        totallen = CHECKSUM_LENGTH + DATA_LENGTH + DTYPE_LENGTH + SEQNO_LENGTH;
        chksumno = checksum(&tmp[CHECKSUM_LENGTH + DATA_LENGTH], DTYPE_LENGTH + SEQNO_LENGTH);
        printf("Make ACK Package: CHECKSUM [%d] Length [%d]...Total leng [%d]\n", chksumno, DTYPE_LENGTH + SEQNO_LENGTH, totallen);
        tmp[0] = (chksumno >> 8) & 0xFF;
        tmp[1] = (chksumno) & 0xFF;
        return totallen;
    } else {
        return 0;
    }
}

void analyze_package_header(u8b_t *inbuf, u16b_t &heachecksum, u16b_t &calchecksum, u16b_t &len, u8b_t &type, u8b_t &seqno) {
    //rdt_packet tmp_pckt;
    heachecksum = (inbuf[0] << 8) | (inbuf[1] & 0xFF);
    //printf("Send Checksum [%d]\n", heachecksum);
    len = (inbuf[CHECKSUM_LENGTH + 0] << 8) | (inbuf[CHECKSUM_LENGTH + 1] & 0xFF);
    type = inbuf[CHECKSUM_LENGTH + DATA_LENGTH];
    seqno = inbuf[CHECKSUM_LENGTH + DATA_LENGTH + DTYPE_LENGTH];
    if (type == DATA) {
        calchecksum = checksum(&inbuf[CHECKSUM_LENGTH + DATA_LENGTH], len + DTYPE_LENGTH + SEQNO_LENGTH);
    } else if (type == ACK) {
        calchecksum = checksum(&inbuf[CHECKSUM_LENGTH + DATA_LENGTH], DTYPE_LENGTH + SEQNO_LENGTH);
    }
    return;
}
#endif
