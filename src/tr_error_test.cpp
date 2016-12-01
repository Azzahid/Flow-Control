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

typedef unsigned char      byte;    // Byte is a char
typedef unsigned short int word16;  // 16-bit word is a short int
typedef unsigned int       word32;

bool checkack(std::vector<bool> ack);
unsigned checksum(void *buffer, size_t len, unsigned int seed);
Byte * stringToArrayOfBytes(string a);

bool status = false;
char x[2];
static int *acknowledged;
int totalframe;
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
		char *ack;
		int windowsize;

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
		/* Initialize XON/XOFF flags */

		 //sending request for windowsize.
		cout << "\nSending request for window size.\n" << endl;
 		sendto(fd,"REQUEST FOR WINDOWSIZE.",sizeof("REQUEST FOR WINDOWSIZE."),0,(struct sockaddr*)&remaddr,slen);

		//obtaining windowsize.
		printf("\nWaiting for the windowsize.\n");
		recvfrom(fd,(int*)&windowsize,sizeof(windowsize),0,(sockaddr*)&remaddr,&addrlen);
		printf("\nThe windowsize obtained as:\t%d\n",windowsize);
		//windowsize = 3;
		/* Create child process */
		pid_t pid = fork();

		if (pid == 0)
		{
			// child process
	        while ( true ) {
			/* Call q_get */
			    struct timeval t;
			    t.tv_sec = 0;
			    t.tv_usec = 300000;
				if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,&t,sizeof(t)) < 0) {
				    perror("Error");
				}
				if(recvfrom(fd, x, sizeof(x), 0, (struct sockaddr *)&remaddr, &addrlen)>0){
					if(*x==XOFF){
						printf("XOFF Diterima\n");
						int asd = recvfrom(fd, x, sizeof(x), 0, (struct sockaddr *)&remaddr, &addrlen);
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
			int count = 0;
			string frame("");
			while (infile.get(c)) {
				if(c != EOF){
					frame.push_back(c);
				}
			}
			//cout << frame << frame.length() << endl;
			totalframe = (frame.length()+framesize-1)/framesize;
			//cout << totalframe << endl;
			string arrframe[totalframe];
			string msg[totalframe];
			string sumarray[totalframe];
			std::vector<bool> ack;
			for(int m=0; m<totalframe;m++){
				ack.push_back(false);
			}
			//make data frame
			string temp("");
			for(int m=0; m<frame.length(); m++){
				temp.push_back(frame[m]);
				if(temp.length() == framesize || m == frame.length()-1){
					arrframe[count] = temp;
					//cout << arrframe[count] << endl;
					temp = "";
					count++;
				}
			}
			//make sumarray
			Byte * tmp;
			unsigned tmpsum;
			for(int m =0; m<totalframe;m++){
				tmp = stringToArrayOfBytes(arrframe[m]);
				tmpsum = checksum(tmp, arrframe[m].length(),0);
				sumarray[m] = to_string(tmpsum);
				//cout <<"isi tmp " << tmp << endl;
			}
			sumarray[10] = "12";
			//make message
			for(int m=0; m<totalframe;m++){
				msg[m].push_back(SOH);
				msg[m].append(to_string(m+1));
				msg[m].push_back(STX);
				msg[m].append(arrframe[m]);
				msg[m].push_back(ETX);
				msg[m].append(sumarray[m]);
				// cout << msg[m] << endl;

			}
			

	/*		for(int i = 0; i<totalframe;i++){
				cout << arrframe[i] << endl;
				cout << arrframe[i].size() << endl;
				//cout << sumarray[i]<<endl;
 			}	
*/
			int framenum = 0;
			string nakfile;
			int m=0;
			while(!checkack(ack)){//paket masih belum di ack semua
				waitpid(pid,NULL,WUNTRACED);
				int numinit = framenum;
				count = 0;
				//make the frame
				string a; 
				while(count<windowsize){
					if(framenum<totalframe){
						a.append(msg[framenum]);
					}
					count++;
					framenum++;
				}
				a.push_back('\0');
				cout << "Sending window : " << a << endl;
				if(sendto(fd, a.data(), a.size(),0,(struct sockaddr *)&remaddr, slen)<0){
					cout << "error" << endl;
				}
				//send frame
				//wait ack
				Byte x [5*windowsize];
				struct timeval t;
			    if(framenum-1 <= windowsize){
			    	sleep(1);
			    }
				vector<bool> faultframe;
				faultframe.push_back(false);
				while(!checkack(faultframe)){
					faultframe.clear();
					vector<int> faultnum;
					string datas;
					t.tv_sec = 3;
				    t.tv_usec = 0;
					if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,&t,sizeof(t)) < 0) {
					    perror("Error");
					}
					ssize_t ackrecv = 1;
					ackrecv = recvfrom(fd,x,sizeof(x), 0, (struct sockaddr *)&remaddr, &addrlen);
					if(ackrecv>0){
						int i =0;
						cout << "ACK received" <<endl;
						while(i<sizeof(x)){
							if(x[i]=='\0'){
								break;
							}else if(x[i]==NAK || x[i]==ACK){
								if(x[i]==ACK){
									faultframe.push_back(true);
								}else{
									faultframe.push_back(false);
								}
								i++;
								string temp("");
								while(x[i]!=NAK && x[i]!=ACK && x[i]!='\0'){
									temp.push_back(x[i]);
									i++;
								}
								faultnum.push_back(stoi(temp));
								count++;
							}else if (x[i] == Endfile){
								break;
							}else{
								i++;
								perror("Format is wrong sending quiting");
								exit(0);
							}
						}
						if(!checkack(faultframe)){
							cout << "Error: NAK for, ";
						}
						for(i = 0; i<faultframe.size();i++){
							if(!faultframe[i]){
								cout << faultnum[i] << " ";
								Byte * tmpx;
								unsigned tmpsumx;
								tmpx = stringToArrayOfBytes(arrframe[faultnum[i]-1]);
								tmpsumx = checksum(tmpx, arrframe[faultnum[i]-1].length(),0);
								sumarray[faultnum[i]-1] = to_string(tmpsumx);
								msg[faultnum[i]-1].clear();
								msg[faultnum[i]-1].push_back(SOH);
								msg[faultnum[i]-1].append(to_string(faultnum[i]-1+1));
								msg[faultnum[i]-1].push_back(STX);
								msg[faultnum[i]-1].append(arrframe[faultnum[i]-1]);
								msg[faultnum[i]-1].push_back(ETX);
								msg[faultnum[i]-1].append(sumarray[faultnum[i]-1]);
								datas.append(msg[faultnum[i]-1]);
								break;
							}
							ack[faultnum[i]-1] = faultframe[i];
						}
						if(!checkack(faultframe)){
							cout << endl;
						}
						cout << endl;
					}else if(ackrecv<0){
						for(i = numinit; i<framenum;i++){
							if(i < totalframe && ack[i]){
								datas.append(msg[i]);
							}
						}
						faultframe.push_back(false);
						cout << "Timeout: send all" <<endl;
					}
					if(datas.size()>1){
						cout << "Re-Sending window" << datas << endl;
						if(sendto(fd, datas.data(), datas.size(),0,(struct sockaddr *)&remaddr, slen)<0){
							cout << "error" << endl;
						}
					}
					if(checkack(ack)){
						break;
					}
					// int axs;
					// cin >> axs;
				}
				kill(pid,SIGCONT);
				//cek ack
				//break;
			}
			/*waitpid(pid,NULL,WUNTRACED);
			Byte * cx = new Byte(static_cast<unsigned char>(c));
			sendto(fd, cx, sizeof(cx),0,(struct sockaddr *)&remaddr, slen);
			cout << "Mengirim byte ke-"<<count<<": '"<<c<<"'\n";
			kill(pid,SIGCONT);
			waitpid(pid,NULL,WUNTRACED);
			kill(pid,SIGCONT);
			count++;
			munmap(acknowledged, sizeof *acknowledged);
			infile.close();*/
			Byte *cx = new Byte(Endfile);
			sendto(fd, cx, sizeof(cx),0,(struct sockaddr *)&remaddr, slen);
			
		}
		else
		{
			// fork failed
			printf("fork() failed!\n");
			return 1;
		}
		
		//cout << frame[0] << endl;
		//cout << isifile << isifile.length() << endl;
	}
	return 0;
}

bool checkack(std::vector<bool> ack){
	for(int i =0; i<ack.size(); i++){
		if(!ack[i]){
			//cout << "false" << i <<endl;
			return false;
		}
		//cout << "true, " << i <<endl;
	}
	return true;
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
	Byte * result = new Byte[a.length()+1];
	for(int i = 0; i<a.length();i++){
		result[i] = a[i];
	}
	result[a.length()]= '\0';  
	return result;
}