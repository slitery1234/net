#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 8192


int main(int argc, char **argv){

    int i, pid, listenfd, socketfd;
    int length;
    int reuse=0;
	static struct sockaddr_in cli_addr;
    static struct sockaddr_in serv_addr;
	printf("Start of Server\n");
	    
	//prevent zombie
    signal(SIGCLD, SIG_IGN);

    //open socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd<0)
        exit(3);
	if ((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) < 0){
		perror("setsockopet error");
	}

	//set net
    serv_addr.sin_family = AF_INET;
    //set IP
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// set port
    serv_addr.sin_port = htons(80);
	
    /* 開啟網路監聽器 */
    if (bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr))<0){
		
		perror("bind");
		exit(3);
	}
	
    
	/* 開始監聽網路 */
    if (listen(listenfd,64)<0){
		
		perror("listen");
		exit(3);
	}
	
}
