#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 1024
#define MAXDATASIZE 100

char sendbuf[10240];
char recvbuf[10240];
char name[100];
int fd;
char board[9];
int turn = 0;
char sign;
char inviter[100];

void help(){
	printf(" --- Menu --- \n");
	printf("To change your name, Press: 1\n");
    printf("To list all online players, Press: 2\n");
    printf("To invite someone start a game, Press: 3\n");
    printf("Type \"logout\" to logout\n");
	printf("To check Menu again, Press: 0\n\n");
}

void print_board(char *board){
    printf("┌───┬───┬───┐        ┌───┬───┬───┐\n");
    printf("│ Q │ W │ E │        │ %c │ %c │ %c │\n", board[0], board[1], board[2]);
    printf("├───┼───┼───┤        ├───┼───┼───┤\n");
    printf("│ A │ S │ D │        │ %c │ %c │ %c │\n", board[3], board[4], board[5]);
    printf("├───┼───┼───┤        ├───┼───┼───┤\n");
    printf("│ Z │ X │ C │        │ %c │ %c │ %c │\n", board[6], board[7], board[8]);
    printf("└───┴───┴───┘        └───┴───┴───┘\n");
}


// modify chess board, and fill "sendbuf" with package format.
void write_on_board(char *board, int location){
    
    board[location] = sign;
    sprintf(sendbuf, "7  %c %c %c %c %c %c %c %c %c\n", board[0], \
        board[1],board[2],board[3],board[4],board[5],board[6],board[7],board[8]);
}



// Only handle message from server to client.
void pthread_recv(void* ptr)
{
    int instruction;
    while(1)
    {
        memset(sendbuf,0,sizeof(sendbuf));
        instruction = 0;
        // recvbuf is filled by server's fd.
        if ((recv(fd,recvbuf,MAXDATASIZE,0)) == -1)
        {
            printf("recv() error\n");
            exit(1);
        }
        sscanf (recvbuf,"%d",&instruction);
        switch (instruction)
        {
            case 2: {
                printf("%s\n", &recvbuf[2]); // Print the message behind the instruction.
                break;
            }
            case 4: {
                sscanf(recvbuf,"%d %s",&instruction, inviter);
                printf("%s\n", &recvbuf[2]); // Print the message behind the instruction.
                break;
            }
            case 6: {
				char hoster[100];
				char dueler[100];
				sscanf(recvbuf,"6 %d %s %s\n",&turn,hoster,dueler);
                for(int i=0;i<9;i++)
					board[i] = '*';
				printf(" --- DUEL START! ---\n");
                printf("* means the block is still empty\n");
                printf("%s uses O\n",hoster);
                printf("%s uses X\n",dueler);
                printf("%s goes first!\n",hoster);
                printf("Please enter the character on the block which you want to use\n");
                print_board(board);
				if(turn)
					sign = 'O';
				else
					sign = 'X';

				break;
            }
            case 8: {
                
                sscanf (recvbuf,"%d %d %c %c %c %c %c %c %c %c %c",&instruction,&turn, \
                    &board[0],&board[1],&board[2],&board[3],&board[4],&board[5],&board[6], \
                        &board[7],&board[8]);
                print_board(board);
                printf("%s\n", recvbuf+22);
				if(turn == 1){
                	printf("Please enter the character on the block which you want to use.\n");
				}
				else if(turn != 2){}
				else{
					printf(" --- END DUEL --- \n");
					help();
					sign = 0;
				}
				break;
            }
            case -1:
		printf("This name is already login, you should use another name.\n");
		exit(1);
            default:
                break;
        }   

        memset(recvbuf,0,sizeof(recvbuf));
    }
}



int main(int argc, char *argv[]){
    
	int  numbytes;
    char buf[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in server;
	turn = 2;
	sign = 0;
    if (argc !=2){
        printf("Usage: %s <IP Address>\n",argv[0]);
        exit(1);
    }


    if ((he=gethostbyname(argv[1]))==NULL){
        perror("gethostbyname");
		exit(1);
    }

    if ((fd=socket(AF_INET, SOCK_STREAM, 0))==-1){
        perror("socket");
    	exit(1);
	}

    bzero(&server,sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr = *((struct in_addr *)he->h_addr);
    
	if(connect(fd, (struct sockaddr *)&server,sizeof(struct sockaddr))==-1){
        perror("connect");
		exit(1);
    }

    // First, Add User.
    printf("connect success\n");
    printf("Pleace enter your user name:");
    fgets(name,sizeof(name),stdin);
    char package[100];
    sprintf(package, "1 %s", name);
	send(fd, package, (strlen(package)),0);

	
    // Only handle message from server to client. (Goto pthread_recv finction)
    pthread_t tid;
    pthread_create(&tid, NULL, (void*)pthread_recv, NULL);
    
    // how to use your program
    help();
	// Only handle message from client to server.
	while(1){
		memset(sendbuf,0,sizeof(sendbuf)); //clear buf
        
		fgets(sendbuf,sizeof(sendbuf),stdin);   // Input instructions
		int location;

		// Logout
        if(strcmp(sendbuf,"logout\n")==0){          
            memset(sendbuf,0,sizeof(sendbuf));
            printf("Logout Successfully.\n");
            return 0;
        }

		if(sendbuf[0] == 'Y'||sendbuf[0] == 'N'||sendbuf[0] == 'y'||sendbuf[0] == 'n'){
			sprintf(buf, "5 %c %s", sendbuf[0], inviter);
			sprintf(sendbuf, "%s", buf);
		}
		else if((sendbuf[0] == 'Q') || (sendbuf[0] == 'q')){
			if(turn == 1){
				if(board[0] != '*')
					printf("The place is already used!\nThis program doesn't support cheating\n");
				else
					write_on_board(board, 0);
    	    }else if(turn == 0){
					printf("It's not your turn!\n");
					continue;
			}else if(turn == 2){
					printf("You are not playing a game now!\n");
					continue;
			}
		}
		else if((sendbuf[0] == 'W') || (sendbuf[0] == 'w')){
			if(turn == 1){
				if(board[1] != '*')
					printf("The place is already used!\nThis program doesn't support cheating\n");
				else
					write_on_board(board, 1);
    	    }else if(turn == 0){
					printf("It's not your turn!\n");
					continue;
			}else if(turn == 2){
					printf("You are not playing a game now!\n");
					continue;
			}
		}
		else if((sendbuf[0] == 'E') || (sendbuf[0] == 'e')){
			if(turn == 1){
				if(board[2] != '*')
					printf("The place is already used!\nThis program doesn't support cheating\n");
				else
					write_on_board(board, 2);
    	    }else if(turn == 0){
					printf("It's not your turn!\n");
					continue;
			}else if(turn == 2){
					printf("You are not playing a game now!\n");
					continue;
			}
		}
		else if((sendbuf[0] == 'A') || (sendbuf[0] == 'a')){
			if(turn == 1){
				if(board[3] != '*')
					printf("The place is already used!\nThis program doesn't support cheating\n");
				else
					write_on_board(board, 3);
    	    }else if(turn == 0){
					printf("It's not your turn!\n");
					continue;
			}else if(turn == 2){
					printf("You are not playing a game now!\n");
					continue;
			}
		}
		else if((sendbuf[0] == 'S') || (sendbuf[0] == 's')){
			if(turn == 1){
				if(board[4] != '*')
					printf("The place is already used!\nThis program doesn't support cheating\n");
				else
					write_on_board(board, 4);
    	    }else if(turn == 0){
					printf("It's not your turn!\n");
					continue;
			}else if(turn == 2){
					printf("You are not playing a game now!\n");
					continue;
			}
		}
		else if((sendbuf[0] == 'D') || (sendbuf[0] == 'd')){
			if(turn == 1){
				if(board[5] != '*')
					printf("The place is already used!\nThis program doesn't support cheating\n");
				else
					write_on_board(board, 5);
    	    }else if(turn == 0){
					printf("It's not your turn!\n");
					continue;
			}else if(turn == 2){
					printf("You are not playing a game now!\n");
					continue;
			}
		}
		else if((sendbuf[0] == 'Z') || (sendbuf[0] == 'z')){
			if(turn == 1){
				if(board[6] != '*')
					printf("The place is already used!\nThis program doesn't support cheating\n");
				else
					write_on_board(board, 6);
    	    }else if(turn == 0){
					printf("It's not your turn!\n");
					continue;
			}else if(turn == 2){
					printf("You are not playing a game now!\n");
					continue;
			}
		}
		else if((sendbuf[0] == 'X') || (sendbuf[0] == 'x')){
			if(turn == 1){
				if(board[7] != '*')
					printf("The place is already used!\nThis program doesn't support cheating\n");
				else
					write_on_board(board, 7);
    	    }else if(turn == 0){
					printf("It's not your turn!\n");
					continue;
			}else if(turn == 2){
					printf("You are not playing a game now!\n");
					continue;
			}
		}
		else if((sendbuf[0] == 'C') || (sendbuf[0] == 'c')){
			if(turn == 1){
				if(board[8] != '*')
					printf("The place is already used!\nThis program doesn't support cheating\n");
				else
					write_on_board(board, 8);
    	    }else if(turn == 0){
					printf("It's not your turn!\n");
					continue;
			}else if(turn == 2){
					printf("You are not playing a game now!\n");
					continue;
			}
		}
		else if( sendbuf[0] == '1' || sendbuf[0] == '3'){
			printf("Enter Name:\n");
			fgets(buf,sizeof(buf),stdin);
			sprintf(sendbuf+1, " %s", buf);
		}
        send(fd,sendbuf,(strlen(sendbuf)),0);   // Send instructions to server
		if(strcmp(sendbuf,"0\n") == 0)
			help();

    }
    pthread_join(tid,NULL);
    close(fd);
    return 0;
}
