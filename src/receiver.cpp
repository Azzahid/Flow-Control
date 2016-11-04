/*
* File : T1_rx.cpp
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
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
	Byte c;
	
	/*
	Insert code here to bind socket to the port number given in argv[1].
	*/
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
	
	
	
	/* Initialize XON/XOFF flags */
	
	/* Create child process */
	pid_t pid = fork();
	
	if (pid == 0)
    {
        // child process
        printf("child process\n");
        while(true) {
			//consume thread
		Byte * test = q_get(rxq,&current_byte);
		
		//if count > 0
		if (test != NULL) {
			//front not in 0
			if (rxq->front > 0) {
				if (rxq->data[rxq->front-1] != Endfile && rxq->data[rxq->front-1] != CR && rxq->data[rxq->front-1] != LF) {
					printf("Consuming byte %i: '%c'\n",++con,rxq->data[rxq->front-1]);
				} else if (rxq->data[rxq->front-1] == Endfile) {
					//if endfile
					printf("End of File accepted.\n");
					exit(0);
				}
			} else {
				if (rxq->data[7] != Endfile && rxq->data[7] != CR && rxq->data[7] != LF) {
					printf("Consuming byte %i: '%c'\n",++con,rxq->data[7]);
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
    }
    else if (pid > 0)
    {
        /*** IF PARENT PROCESS ***/
		int count = 1;
		while ( true ) {
			c = *(rcvchar(sockfd, rxq));
			printf("Menerima byte ke-%d.\n", count);
			/* Quit on end of file */
			if (c == Endfile) {
				exit(0);
			}
			count++;
		}
    }
    else
    {
        // fork failed
        printf("fork() failed!\n");
        return 1;
    }
	
	
	/*** ELSE IF CHILD PROCESS ***/
	while ( true ) {
		/* Call q_get */
		/* Can introduce some delay here. */
	}
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
			printf("recvfrom() failed\n");
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
			printf("Received byte %i\n",co);
		}
		
		//if buffer size excess minimum upperlimit
		if (queue->count > MIN_UPPERLIMIT && sent_xonxoff == XON) {
			sent_xonxoff = XOFF;
			send_xoff = truey;
			send_xon = falsey;
			printf("Buffer > minimum upperlimit. Send XOFF.\n");
			char test[2];
			test[0] = XOFF;
			//send XOFF to transmitter
			ssize_t numBytesSent = sendto(sockfd, test, sizeof(test), 4,
				(struct sockaddr *) &remaddr, sizeof(remaddr));
			if (numBytesSent < 0)
				printf("sendto() failed)");
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
			printf("Buffer < maksimum lowerlimit. Send XON\n");
			char test[2];
			test[0] = XON;
			//send XON to transmitter
			ssize_t numBytesSent = sendto(sockfd, test, sizeof(test), 4,
			(struct sockaddr *) &remaddr, sizeof(remaddr));
			if (numBytesSent < 0)
				printf("sendto() failed\n");
		}
		
		return data;
	}
}
