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

Byte rxbuf[RXQSIZE];
QTYPE rcvq = { 0, 0, 0, RXQSIZE, rxbuf };
QTYPE *rxq = &rcvq;
Byte sent_xonxoff = XON;
Boolean send_xon = falsey , send_xoff = falsey ;

/* Socket */
int sockfd; // listen on sock_fd

/* Functions declaration */
static Byte *rcvchar( int sockfd, QTYPE *queue);
static Byte *q_get(QTYPE *, Byte *);

int main( int argc, char *argv[])
{
	Byte c;
	/*
	Insert code here to bind socket to the port number given in argv[1].
	*/
	struct sockaddr_in myaddr;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	
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
	printf("Binding pada %s:%d\n", inet_ntoa(myaddr.sin_addr) , port);
	
	
	
	/* Initialize XON/XOFF flags */
	
	/* Create child process */
	pid_t pid = fork();
	
	if (pid == 0)
    {
        // child process
        printf("child process\n");
        while(true) {
			
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
static Byte *rcvchar( int sockfd, QTYPE *queue)
{
	/*
	Insert code here.
	Read a character from socket and put it to the receive
	buffer.
	If the number of characters in the receive buffer is above
	certain level, then send XOFF and set a flag (why?).
	Return a pointer to the buffer where data is put.
	*/
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	rcvq.count++;
	printf("%d", rcvq.count);
	recvlen = recvfrom(sockfd, rxbuf, RXQSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
	return rxbuf;	
}

/* q_get returns a pointer to the buffer where data is read or NULL if
* buffer is empty.
*/
static Byte *q_get(QTYPE *queue, Byte *data)
{
	Byte *current;
	/* Nothing in the queue */
	if (!queue->count) return (NULL);
	/*
		Insert code here.
		Retrieve data from buffer, save it to "current" and "data"
		If the number of characters in the receive buffer is below
		certain level, then send XON.
		Increment front index and check for wraparound.
	*/
}
