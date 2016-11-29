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
#include <iostream>
#include <vector>

/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 500

/* Define receive buffer size */
#define RXQSIZE 15

/* Define minimum upperlimit */
#define MIN_UPPERLIMIT 10

/* Define maximum lowerlimit */
#define MAX_LOWERLIMIT 1

/*Define window size*/
#define windowsize 3

using namespace std;
Byte rxbuf[RXQSIZE];
QTYPE rcvq = { 0, 0, 0, RXQSIZE, rxbuf };
QTYPE *rxq = &rcvq;
Byte sent_xonxoff = XON;
Boolean send_xon = falsey , send_xoff = falsey ;

/* Socket */
int sockfd; // listen on sock_fd

void* consumerthread(void *threadArgs);
unsigned checksum(void *buffer, size_t len, unsigned int seed);
Byte * stringToArrayOfBytes(string a);

struct sockaddr_in remaddr;	/* remote address */
socklen_t addrlen = sizeof(remaddr);		/* length of addresses */

/* Functions declaration */
static Byte *rcvchar( int sockfd, QTYPE *queue);
static Byte *q_get(QTYPE *, Byte *);

/* Global Variable */
int co = 0;
int con = 0;
Byte * current_byte;
int SIZE = 0;

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
		
		
		SIZE = windowsize*(6+framesize)+1;
		char a[SIZE];
		if(recvfrom(sockfd, a, sizeof(a), 0,(struct sockaddr *) &remaddr, &addrlen) < 0){
			perror("failed receive");
			return 0;
		}
		//cout << a << endl;
		
		//send frame size
		//send window size
		int * ax = new int(windowsize);
		sendto(sockfd, ax, sizeof(ax), 4,
			(struct sockaddr *) &remaddr, sizeof(remaddr));
		/* Membuat thread baru */
		pthread_create(&consumert,NULL,consumerthread,NULL);
		
		/*** Main Thread Code ***/
		while(true) {
			//read byte from socket
			current_byte = rcvchar(sockfd, rxq);
			//cout << "catx" <<endl;
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
		//cout << "caty" << endl;
		Byte * test = q_get(rxq,current_byte);
		
		//if count > 0
		if (test != NULL) {
			//front not in 0
			//cout << "catz" << endl;
			if (rxq->front > 0) {
				//cout << "cata" << endl;
				if (rxq->data[rxq->front-1] != Endfile && rxq->data[rxq->front-1] != CR && rxq->data[rxq->front-1] != LF) {
					printf("Mengkonsumi byte ke-%d: '%c'\n",++con,rxq->data[rxq->front-1]);
				} else if (rxq->data[rxq->front-1] == Endfile) {
					//if endfile
					printf("End of File accepted.\n");
					exit(0);
				}
			} else {
				//cout << "catb" << endl;
				if (rxq->data[RXQSIZE-1] != Endfile && rxq->data[RXQSIZE-1] != CR && rxq->data[RXQSIZE-1] != LF) {
					printf("Mengkonsumsi byte ke-%d: '%c'\n",++con,rxq->data[RXQSIZE-1]);
				} else if (rxq->data[RXQSIZE-1] == Endfile) {
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

Byte tes[windowsize*(10+framesize)+1];
static Byte *rcvchar( int sockfd, QTYPE *queue)
{
	Byte *buffer;
	vector<int> framenum;
	vector<string> data;
	vector<unsigned> sumarray;
	vector<bool> faultframe;

	if (!send_xoff) {
		//read from socket & push it to queue
		bzero((char*)tes,sizeof(tes));
		ssize_t numBytesRcvd = recvfrom(sockfd, tes, sizeof(tes), 0,(struct sockaddr *) &remaddr, &addrlen);
		//process data from vector
		//cout << tes << endl;
		
//		cou//cek data
		int count = 0;
		int i = 0;
		bool format = true;
		//get data to array
		while(i<sizeof(tes)){
			if(tes[i]=='\0'){
				break;
			}else if(tes[i]==SOH){
				i++;
				string numtemp;
				while(tes[i]!=STX){
					numtemp.push_back(tes[i]);
					//cout << tes[i] <<", "<< i << endl;
					i++;
				}
				framenum.push_back(stoi(numtemp));
				//cout << numtemp << endl;
				i++;
				string datatemp;
				while(tes[i]!=ETX){
					datatemp.push_back(tes[i]);
					//cout << tes[i] << ", "<< i << endl;
					i++;
				}
				data.push_back(datatemp);
				i++;
				string sumtemp;
				while(tes[i]!=SOH && tes[i]!='\0'){
					sumtemp.push_back(tes[i]);
					i++;
				}
				sumarray.push_back(stoi(sumtemp));
				//count++;
			}else if (tes[i] == Endfile){
				break;
			}else{
				format = false;
				perror("Format is wrong sending NAK of all data`");
			}
		}
		// for(int i=0; i<windowsize;i++){
			// cout << framenum[i] <<endl;
			// cout << data[i] <<endl;
			// cout << sumarray[i] << endl;
		// }
		//cout << "fish" <<endl;
		if(format){
			if(tes[i]!=Endfile){
				//cek sumarray
				Byte * tempb;
				unsigned temp;
				string acknowledged;
				for(int i = 0; i<data.size();i++){
					tempb = stringToArrayOfBytes(data[i]);
					temp = checksum(tempb, data[i].length(),0);
					if(temp == sumarray[i]){
						faultframe.push_back(true);
						//cout << sizeof(tempb) << endl;
					}else{
						faultframe.push_back(false);
						//cout << "--" << temp << "--" << tempb << "--" << i << "--" 
					//	<< data[i].length() << "--" << data[i] 
				//		<< "--" << sumarray[i] <<"--" << *tempb << endl;
					}
				}
				if (numBytesRcvd < 0) {
					//if error
					printf("recvfrom() gagal!\n");
				}
				for(int i = 0; i<data.size();i++){
					if(faultframe[i]){
						acknowledged.append(data[i]);
					}
				}
				//cout << acknowledged << endl;
				//fill the circular
				for(int i =0; i<acknowledged.length(); i++){
					queue->data[queue->rear] = acknowledged[i];
					queue->count++;
					if (queue->rear < RXQSIZE-1) {
						queue->rear++;
					} else {
						queue->rear = 0;
					}
				}
				
				if (tes[0] != Endfile && tes[0] != CR && tes[0] != LF) {
					//printf("Menerima frame ke-");
					for (int i= 0; i<framenum.size();i++){
						if(faultframe[i]){
							cout << framenum[i] << " ";
						}
					}
					//cout << endl;
				}
				char test[2];
				vector<string> ack;
				//kirim ack
				string sendingack("");
				for(int i =0; i<faultframe.size(); i++){
					ack.push_back(string());
					if(faultframe[i]){
						ack[i].push_back(ACK);
					}else{
						ack[i].push_back(NAK);
					}
					ack[i].append(to_string(framenum[i]));
					//ack[i].append(to_string(sumarray[i]));
					sendingack = sendingack.append(ack[i]);
				}
				sendingack.push_back('\0');
				cout << sendingack <<endl;
				//printf("Mengirim ACK.\n");
				ssize_t numBytesSent = sendto(sockfd, sendingack.data(), sendingack.size(), 4,
						(struct sockaddr *) &remaddr, sizeof(remaddr));
				if (numBytesSent < 0)
					printf("sendto() gagal!");
				//if buffer size excess minimum upperlimit
				if (queue->count > MIN_UPPERLIMIT && sent_xonxoff == XON) {
					sent_xonxoff = XOFF;
					send_xoff = truey;
					send_xon = falsey;
					printf("Buffer > minimum upperlimit.\n Mengirim XOFF.\n");
					test[0] = XOFF;
					//send XOFF to transmitter
					ssize_t numBytesSent = sendto(sockfd, test, sizeof(test), 4,
						(struct sockaddr *) &remaddr, sizeof(remaddr));
					if (numBytesSent < 0)
						printf("sendto() gagal!");
				}
				Byte *rt;
				rt = stringToArrayOfBytes(acknowledged);
				cout << rt << ", fish, " << queue->count << endl;
				return rt;
			}else{
				queue->data[queue->rear] = tes[i];
					queue->count++;
					if (queue->rear < RXQSIZE-1) {
						queue->rear++;
					} else {
						queue->rear = 0;
					}
			}
		}else{
			Byte * rt;
			string error = "error";
			rt = stringToArrayOfBytes(error);
			return rt;
		}
		
	} else {
		//cout << "catburn"<< endl;
		Byte * rt;
		string error = "0";
		rt = stringToArrayOfBytes(error);
		return rt;
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
			//cout << "cat" <<endl;
			//obtain buffer
			if (queue->count > 0) {
				(*data) = queue->data[queue->front];
				queue->count--;
				if (queue->front < RXQSIZE-1) {
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

unsigned checksum(void *buffer, size_t len, unsigned int seed)
{
      unsigned char *buf = (unsigned char *)buffer;
      size_t i;

      for (i = 0; i < len; ++i)
            seed += (unsigned int)(*buf++);
      return seed;
}

Byte * stringToArrayOfBytes(string a){
	//cout << a << endl;
	Byte * result = new Byte[a.length()];
	for(int i = 0; i<a.length();i++){
		result[i] = a[i];
	}
	result[a.length()]= '\0';  
	return result;
}