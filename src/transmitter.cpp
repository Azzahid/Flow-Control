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
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string> //std::string
#include <vector>
#include <sys/mman.h>
using namespace std;

bool status = false;
char x[2];
static int *acknowledged;
int main(int argc, char *argv[])
{
	if (argc < 4 || argc > 4) {
		printf("Usage : transmitter [IP Target] [Port Target] [File to Send]");
	} else {
		struct sockaddr_in myaddr, remaddr;
		int fd, i, slen=sizeof(remaddr);
		char *server = argv[1];	
		char buf[MAXLEN];
		int port = atoi(argv[2]);
		string text_file = argv[3];
		socklen_t addrlen = sizeof(remaddr);
		string isifile ("");
		std::vector<string> frame;
		char *ack;

		printf("Membuat socket untuk koneksi ke %s:%d ...\n",server,port);
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
		int kx = 0;
		acknowledged =(int *) mmap(NULL, sizeof *acknowledged, PROT_READ | PROT_WRITE, 
                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
		/* now let's send the messages */
		*acknowledged = 1;
		/* Initialize XON/XOFF flags */
	
		/* Create child process */
		pid_t pid = fork();

		if (pid == 0)
		{
			// child process
	        while ( true ) {
			/* Call q_get */
			    struct timeval t;
			    t.tv_sec = 0;
			    t.tv_usec = 100000;
				if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,&t,sizeof(t)) < 0) {
				    perror("Error");
				}
				if(recvfrom(fd, x, sizeof(x), 0, (struct sockaddr *)&remaddr, &addrlen)>0){
					if(*x==XOFF){
						printf("XOFF Diterima\n");
						*acknowledged = 0;
						while(true){
							if(recvfrom(fd,x,sizeof(x), 0, (struct sockaddr *)&remaddr, &addrlen)>0){
								if(*x == XON){
									printf("XON Diterima\n");
									break;
								}
							}else{
								//sleep(10);
								printf("Menunggu XON...\n");
							}
						}
					}else{
						if(*x==ACK){
							*acknowledged = 0;
							if(*acknowledged==0){
								printf("ACK Diterima...\n");
							}
						}else if (*x == NAK){
							*acknowledged = 1;
						}
					}
				}
				raise(SIGSTOP);
			}
		}
		else if (pid > 0)
		{
			// parent process
			ifstream infile;
			infile.open(text_file.c_str());
			char c;
			int count = 1;
			while (infile.get(c)) {
				while(*acknowledged==1){
					waitpid(pid,NULL,WUNTRACED);
					Byte * cx = new Byte(static_cast<unsigned char>(c));
					sendto(fd, cx, sizeof(cx),0,(struct sockaddr *)&remaddr, slen);
					cout << "Mengirim byte ke-"<<count<<": '"<<c<<"'\n";
					kill(pid,SIGCONT);
					waitpid(pid,NULL,WUNTRACED);
					kill(pid,SIGCONT);
				}
				count++;
				*acknowledged=1;
			}
			munmap(acknowledged, sizeof *acknowledged);
			infile.close();
			Byte * cx = new Byte(Endfile);
			sendto(fd, cx, sizeof(cx),0,(struct sockaddr *)&remaddr, slen);
			
		}
		else
		{
			// fork failed
			printf("fork() failed!\n");
			return 1;
		}
		for(int i = 0; i<frame.size();i++){
				cout << frame[i] << endl;
		}	
		//cout << frame[0] << endl;
		//cout << isifile << isifile.length() << endl;
	}
	return 0;
}

