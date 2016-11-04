/**
 * File : transmiter.cpp
 * */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> // fork
#include <string.h>
#include <netdb.h>
#include <sys/socket.h> // inet_aton
#include <netinet/in.h> // inet_aton
#include <arpa/inet.h> // inet_aton
#include <iostream> // cin cout
#include <fstream> // ifstream
#include "dcomm.h"

using namespace std;

int main(int argc, char *argv[])
{
	if (argc < 3 || argc > 4) {
		printf("Usage : transmitter [IP Target] [Port Target] [File to Send]");
	} else {
		struct sockaddr_in myaddr, remaddr;
		int fd, i, slen=sizeof(remaddr);
		char *server = argv[0];	
		char buf[MAXLEN];
		int port = atoi(argv[1]);
		string text_file = argv[2];
		
		printf("Membuat socket untuk koneksi ke %s:%d ...",server,port);
		/* Create a Socket */
		if ((fd=socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("cannot create socket\n");
			return 0;
		}
		
		/* bind it to all local addresses and a specific port */	
		memset((char *)&myaddr, 0, sizeof(myaddr));
		myaddr.sin_family = AF_INET;
		myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		myaddr.sin_port = htons(0);	
		
		if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
			perror("bind failed");
			return 0;
		}
		
		/* now define remaddr, the address to whom we want to send messages */
		/* For convenience, the host address is expressed as a numeric IP address */
		/* that we will convert to a binary format via inet_aton */
		memset((char *) &remaddr, 0, sizeof(remaddr));
		remaddr.sin_family = AF_INET;
		remaddr.sin_port = htons(port);
		if (inet_aton(server, &remaddr.sin_addr)==0) {
			fprintf(stderr, "inet_aton() failed\n");
			exit(1);
		}
		
		/* now let's send the messages */

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
			// parent process
			ifstream infile;
			infile.open(text_file.c_str());
			char c;
			while (infile.get(c)) {
				
			}
			infile.close();
		}
		else
		{
			// fork failed
			printf("fork() failed!\n");
			return 1;
		}
	}
	return 0;
}
