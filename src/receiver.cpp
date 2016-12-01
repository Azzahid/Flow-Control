/*
* File : receiver.c
* Source and edit from : https://github.com/masphei/flow-control-udp/blob/c1e4cd9e7b18a04e5d0465f007c74142c8b75a5b/Control.c 
* Reference source code credit to : masphei
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "dcomm.h"

/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 500

/* Define receive buffer size */
#define RXQSIZE 8

/* Define minimum upperlimit */
#define MIN_UPPERLIMIT 4

/* Define maximum lowerlimit */
#define MAX_LOWERLIMIT 1

Byte rxbuf[RXQSIZE];
QTYPE rcvq = { 0, 0, 0, RXQSIZE, rxbuf };
QTYPE *rxq = &rcvq;
Byte sent_xonxoff = XON;
Boolean send_xon = falsey , send_xoff = falsey ;

/* Socket */
int sockfd; // listen on sock_fd

void* consumerthread(void *threadArgs);

struct sockaddr_in remaddr;	/* remote address */
socklen_t addrlen = sizeof(remaddr);		/* length of addresses */

/* Functions declaration */
static Byte *rcvchar( int sockfd, QTYPE *queue);
static Byte *q_get(QTYPE *, Byte *);

/* Global Variable */
int co = 0;
int con = 0;
Byte current_byte;

int main( int argc, char *argv[])
{
	if (argc < 2 || argc>2) {
		printf("Usage : receiver [PORT]\n");
	} else {
	
		pthread_t consumert;
		struct sockaddr_in myaddr;	/* our address */
		
		/* create a UDP socket */
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("cannot create socket\n");
			return 0;
		}
		
		/* bind the socket to any valid IP address and a specific port */
		int port = atoi(argv[1]);
		memset((char *)&myaddr, 0, sizeof(myaddr));
		myaddr.sin_family = AF_INET;
		myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		myaddr.sin_port = htons(port);

		if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
			perror("bind failed");
			return 0;
		}
		
		printf("Binding pada %s:%d\n ...", inet_ntoa(myaddr.sin_addr) , port);
		
		/* Membuat thread baru */
		pthread_create(&consumert,NULL,consumerthread,NULL);
		
		/*** Main Thread Code ***/
		while(true) {
			//read byte from socket
			current_byte = *(rcvchar(sockfd, rxq));
		}
		
		//close socket
		close(sockfd);
		return 0;
	}
}

/*** Consumer Thread Code ***/
void* consumerthread(void *threadArgs)
{
	while (true) {
		//consume thread
		Byte * test = q_get(rxq,&current_byte);
		
		//if count > 0
		if (test != NULL) {
			//front not in 0
			if (rxq->front > 0) {
				if (rxq->data[rxq->front-1] != Endfile && rxq->data[rxq->front-1] != CR && rxq->data[rxq->front-1] != LF) {
					printf("Mengkonsumi byte ke-%d: '%c'\n",++con,rxq->data[rxq->front-1]);
				} else if (rxq->data[rxq->front-1] == Endfile) {
					//if endfile
					printf("End of File accepted.\n");
					exit(0);
				}
			} else {
				if (rxq->data[7] != Endfile && rxq->data[7] != CR && rxq->data[7] != LF) {
					printf("Mengkonsumsi byte ke-%d: '%c'\n",++con,rxq->data[7]);
				} else if (rxq->data[7] == Endfile) {
					//if endfile
					printf("End of File accepted.\n");
					exit(0);
				}
			}
		}
		//delay
		usleep(DELAY*1000);
	}
	return NULL;
}

Byte tes[2];
static Byte *rcvchar( int sockfd, QTYPE *queue)
{
	Byte *buffer;
	if (!send_xoff) {
		//read from socket & push it to queue
		ssize_t numBytesRcvd = recvfrom(sockfd, tes, sizeof(tes), 0,(struct sockaddr *) &remaddr, &addrlen);
			
		if (numBytesRcvd < 0) {
			//if error
			printf("recvfrom() gagal!\n");
		} else {
			//fill the circular
			queue->data[queue->rear] = tes[0];
			queue->count++;
			if (queue->rear < 7) {
				queue->rear++;
			} else {
				queue->rear = 0;
			}
			co++;
		}
		
		if (tes[0] != Endfile && tes[0] != CR && tes[0] != LF) {
			printf("Menerima byte ke-%d.\n",co);
		}
		
		//if buffer size excess minimum upperlimit
		if (queue->count > MIN_UPPERLIMIT && sent_xonxoff == XON) {
			sent_xonxoff = XOFF;
			send_xoff = truey;
			send_xon = falsey;
			printf("Buffer > minimum upperlimit.\n Mengirim XOFF.\n");
			char test[2];
			test[0] = XOFF;
			//send XOFF to transmitter
			ssize_t numBytesSent = sendto(sockfd, test, sizeof(test), 4,
				(struct sockaddr *) &remaddr, sizeof(remaddr));
			if (numBytesSent < 0)
				printf("sendto() gagal!");
		}
		
		return &tes[0];
		
	} else {
		*buffer = 0;
		return buffer;
	}	
}

/* q_get returns a pointer to the buffer where data is read or NULL if
* buffer is empty.
*/
static Byte *q_get(QTYPE *queue, Byte *data)
{
	
	if (!queue->count) {
		return (NULL);
	} else {
		do {
			//obtain buffer
			if (queue->count > 0) {
				(*data) = queue->data[queue->front];
				queue->count--;
				if (queue->front < 7) {
					queue->front++;
				} else {
					queue->front = 0;
				}
			}
			//check if data valid
		} while ((*data < 32) && (*data != LF) && (queue->count > 0));
		
		//if count reach the maksimum lowerlimit
		if (queue->count < MAX_LOWERLIMIT && sent_xonxoff == XOFF) {
			sent_xonxoff = XON;
			send_xon = truey;
			send_xoff = falsey;
			printf("Buffer < maksimum lowerlimit.\n Mengirim XON.\n");
			char test[2];
			test[0] = XON;
			//send XON to transmitter
			ssize_t numBytesSent = sendto(sockfd, test, sizeof(test), 4,
			(struct sockaddr *) &remaddr, sizeof(remaddr));
			if (numBytesSent < 0)
				printf("sendto() gagal!\n");
		}
		
		return data;
	}
}
