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
void doGET(long read_datasiz, int fd, char* buffer){

	int buffer_dirlen = 0;
	int buflen = strlen(buffer);
	// directory in GET command
	// save in char* dir
	for(int i=4; i<buflen; i++, buffer_dirlen++)
	{
		if(buffer[i] == ' ')
			break;
		if(buffer[i] == '.')
			continue;
	}
	char* dir;
	dir = malloc(buffer_dirlen+1);
	dir[0] = '.';
	if(buffer_dirlen == 1)
		sprintf(dir, "./home.html");
	else if(buffer[4] == '.')
		strncpy(&dir[1], &buffer[5], buffer_dirlen);
	else
		strncpy(&dir[1], &buffer[4], buffer_dirlen);


	// open file in GET command
	int file_fd;
	if((file_fd = open(dir, O_RDONLY)) == -1)
		write(fd, "Failed to open file.", 19);

	sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
	write(fd, buffer, strlen(buffer));
	int tmp;
	while((tmp = read(file_fd, buffer, BUFSIZE)) > 0)
		write(fd, buffer, tmp);

	// debug 
	//printf("GET:%s\n",buffer);

	close(file_fd);
	free(dir);
	return;

}
void doPOST(long read_datasiz, int fd, char* buffer){
	
	char* ptrS = strstr(buffer, "Content-Length: ");
	ptrS += 16;
	char* ptrE = strstr(ptrS, "\r\n");
	char tmp[BUFSIZE];
	strncpy(tmp, ptrS, (ptrE-ptrS));
	tmp[ptrE-ptrS] = '\0';
	int content_length = atoi(tmp);
	
	//debug
	//printf("%d\n", content_length);
	

	// move ptrS to boundary start
	ptrS = strstr(ptrS, "\r\n--") + 2;
	ptrE = strstr(ptrS, "\r\n");;
	int boundary_length = ptrE - ptrS + 1;
	ptrE = strstr(ptrE+1, "\r\n");
	ptrE = strstr(ptrE+1, "\r\n");
	ptrE = strstr(ptrE+1, "\r\n") + 2;
	// true content length, 6 means /r/n(boundary)--
	content_length -= ((ptrE - ptrS) + (boundary_length + 6));
	
	//debug
	//printf("?%s?\n", ptrE);
	//printf("%d\n", content_length);
	
	char* dir = (char*)malloc(BUFSIZ);
	char* dirS = strstr(buffer, "filename=\"") + 10;
	char* dirE = strstr(dirS, "\"");
	strncpy(tmp, dirS, (dirE-dirS));
	tmp[dirE-dirS] = '\0';
	sprintf(dir, "./upload/%s", tmp);

	//debug
	//printf("d%sd\n", dir);

	int file_fd;
	if((file_fd = open(dir,O_CREAT|O_WRONLY|O_TRUNC|O_SYNC,S_IRWXO|S_IRWXU|S_IRWXG)) == -1)
		write(fd, "Failed to open file.\n", 19);
	
	ptrS = ptrE;
	int len = read_datasiz - (ptrS-buffer);
	if(len > content_length){
		write(file_fd, ptrS, content_length);
		content_length = 0;
	}
	else{
		int siz = write(file_fd, ptrS, len);
		content_length -= siz;
		while(content_length){
			siz = read(fd, buffer, BUFSIZE);
			//debug
			//printf("%s", buffer);
			if(siz > content_length){
				write(file_fd, buffer, content_length);
				content_length = 0;
				break;
			}
			write(file_fd, buffer, siz);
			content_length -= siz;
		}
	}
	close(file_fd);

	file_fd = open("./home.html", O_RDONLY);
	sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
	write(fd, buffer, strlen(buffer));
	while((len = read(file_fd, buffer, BUFSIZE)) > 0)
		write(fd, buffer, len);
	close(file_fd);
	free(dir);
	
	return;

}
void dealrequest(int fd){
	// initial buffer
	char* buffer = (char*)malloc(BUFSIZE);
	memset(buffer, 0, BUFSIZE);

	// read data from browser
	long read_datasiz = read(fd, buffer, BUFSIZE);
	int buflen = strlen(buffer);

	// print the buffer on terminal
	int i;
	if(read_datasiz <= 0)
	{
		fprintf(stderr, "Error: Error at read data from socket.\n");
		perror("Error: ");
		exit(1);
	}
	else
	{
		for(i=0; i<buflen; i++)
			printf("%c",buffer[i]);
	}
	i = 0;
	// deal GET and POST
	if(strncmp(buffer, "GET ", 4) == 0)
	{
		printf("\n////// GET //////\n");
		doGET(read_datasiz, fd, buffer);
	}
	else if(strncmp(buffer, "POST ", 5) == 0)
	{
		printf("\n////// POST //////\n");
		doPOST(read_datasiz, fd, buffer);
	}
	else
	{
		printf("Not GET or POST, ignore...\n");
	}
	free(buffer);
	return;
}

int main(int argc, char** argv)
{
	// save data in storage
	if(chdir("./storage") == -1)
		fprintf(stderr, "Error: Error at change directory");
	// prevent zombie
	// if parent don't deal with SIGCLD, child will wait and become zombie process
	signal(SIGCLD, SIG_IGN);

	int socket_fd; 
	// get socket
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_fd < 0)
	{
		fprintf(stderr, "Error: Error at get socket.\n");
		perror("Error: ");
		exit(1);
	}
	int reuse = 0;
	// set socket option
	if((setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) < 0)
	{
		fprintf(stderr, "Error: Error at set socket option.\n");
		perror("Error: ");
		exit(1);
	}
	

	static struct sockaddr_in client_addr;
	static struct sockaddr_in server_addr;
	// set net, ip address, port
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_port = htons(80);
	
	// assign address to the socket
	if(bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		fprintf(stderr, "Error: Error at bind address.\n");
		perror("Error: ");

		exit(1);
	}
	
	// start listen connection to the socket
	// set maximum request as 128
	if(listen(socket_fd, 128) < 0)
	{
		fprintf(stderr, "Error: Error at listen connection.\n");
		perror("Error: ");
		exit(1);
	}
	
	int length;
	int accept_fd;
	int pid;
	while(1)
	{
		length = sizeof(client_addr);
		accept_fd = accept(socket_fd, (struct sockaddr* )&client_addr, &length);
		if(accept_fd < 0)
		{
			fprintf(stderr, "Error: Error at accept client connection.\n");
			perror("Error: ");
			exit(1);
		}
		// use child process to deal with client request
		if((pid = fork()) < 0)
		{
			fprintf(stderr, "Error at fork child process.\n");
			perror("Error: ");
			exit(1);
		}
		else
		{
			if(pid == 0) 
				dealrequest(accept_fd);
		}
		close(accept_fd);
	}
	
}
