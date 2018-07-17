//NAME: HARRY YEE
////EMAIL: hyee1234@gmail.com
////UID: 704754172


#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "zlib.h"
#include <stdlib.h>



//global variables cause we can (:
struct termios saved_settings;
int pipetoshell[2];
int pipetoterm[2];
pid_t pid;
char * exec_bash[] = {"/bin/bash", NULL}; 
struct pollfd poll_key[1];
struct pollfd poll_ptt[1];
const int cr=0xD;
const int  lf=0xA;
char buffer[256];;
int p;
int op;
int i=0;
int shell_flag = 0;
int portnum;
int comp_flag = 0;

//variables for socket
int sockfd, newsockfd, portno;
socklen_t clilen;
char buffer[256];


//variables for compression and decompresssion
z_stream ClientToServer;
z_stream ServerToClient;


void handler(int signal)
{
	if(signal == SIGPIPE)
	{	
		close(pipetoshell[1]);
		close(pipetoterm[0]);
		exit(0);
	}
}
void restore()
{
	int status;
		if(waitpid(pid,&status,0) == -1)
		{
			fprintf(stderr,"Waitpid error");
		}
		
		int stat = WEXITSTATUS(status);
		int sig = WTERMSIG(status);
		fprintf(stderr,"SHELL EXIT SIGNAL=%d STATUS=%d\n",sig,stat);
	exit(0);	
}
int  decomp(int read_in, char* inbuf, char* outbuf)
{

	ClientToServer.zalloc=Z_NULL;
	ClientToServer.zfree=Z_NULL;
	ClientToServer.opaque=Z_NULL;

    inflateInit(&ClientToServer);

    ClientToServer.avail_in= (uInt) read_in;
    ClientToServer.next_in = (Bytef*) inbuf;
    ClientToServer.avail_out = (uInt) 4000;
    ClientToServer.next_out = (Bytef*) outbuf;

    memset(outbuf,0,4000);
    do{
        inflate(&ClientToServer,Z_SYNC_FLUSH);
    }while(ClientToServer.avail_in > 0);

	return 4000 - ClientToServer.avail_out;

	inflateEnd(&ClientToServer);
}
int comp(int read_in, char* inb, char* outb )
{
    ServerToClient.zalloc=Z_NULL;
    ServerToClient.zfree=Z_NULL;
    ServerToClient.opaque=Z_NULL;

    deflateInit(&ServerToClient, Z_DEFAULT_COMPRESSION);


    ServerToClient.avail_in= (uInt) read_in;
    ServerToClient.next_in = (Bytef*) inb;
    ServerToClient.avail_out = (uInt) 4000;
    ServerToClient.next_out = (Bytef*) outb;

    memset(outb,0,4000);

    do{
        deflate(&ServerToClient,Z_SYNC_FLUSH);
    }while(ServerToClient.avail_in > 0);

    return 4000 - ServerToClient.avail_out;
	deflateEnd(&ServerToClient);
}
void socketinit(int portnum)
{
     struct sockaddr_in serv_addr, cli_addr;
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
        fprintf(stderr,"ERROR opening socket");
     memset((char *) &serv_addr,0 ,sizeof(serv_addr));
     portno = portnum;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
     {         
		fprintf(stderr,"ERROR on binding");
		exit(1);
	 }
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
     if (newsockfd < 0)
          fprintf(stderr,"Error on accept");


}
void parentdirect()
{
	socketinit(portnum);
	poll_key[0].fd = newsockfd;
	poll_ptt[0].fd = pipetoterm[0];
	poll_key[0].events = POLLIN | POLLHUP | POLLERR;
	poll_ptt[0].events = POLLIN | POLLHUP | POLLERR;
	char  buf2[4000];
	char buf3[4000];
	while(1)
	{
		
		if(poll(poll_key,1,0) == -1)
		{
			exit(1);
		}
		if(poll(poll_ptt,1,0) == -1)
		{
			exit(1);
		}
		//keyboard has input
		if(poll_key[0].revents & POLLIN)
		{
			int z;
			if(comp_flag)
			{
				p = read (newsockfd, buf3, 4000);
				z =decomp(p,buf3,buf2);
			}else
			{
				z = read(newsockfd, buf2, 4000);
			}
			for(i = 0; i < z; i++){
				
				switch(buf2[i])
				{	
					case '\004':
						//cut resources to shell
						close(pipetoshell[1]);
						exit(1);
						break;
					case '\003':
						//send kill to child
						if(kill(pid,SIGINT) == -1)
							exit(1);
                                               // exit(1);
						break;
					case '\r':
					case '\n':
                        	//write to bash	
                          	write(pipetoshell[1],&lf,1);
						break;
					default:	
                        write(pipetoshell[1],buf2 + i,1);
						break;
				}
			}
		}
		//shell has output to read
		if(poll_ptt[0].revents & POLLIN)
		{

			p = read(pipetoterm[0],buf2,4000);
			if(comp_flag){
				int z = comp(p,buf2,buf3);
				write(newsockfd,buf3,z);
			}else
			{
				write(newsockfd,buf2,p);
			}
		}

		//check for any errors from the sehll
		if(poll_ptt[0].revents & (POLLHUP | POLLERR))
		{
			fprintf(stderr,"Shell shutdown");
			close(pipetoshell[1]);
			close(pipetoterm[0]);
			close(newsockfd);
			close(sockfd);
			restore();
			exit(1);
		}

	

	}


}	

void pipeconnect()
{

	if(pid == 0)//child
	{
		close(pipetoshell[1]);
		close(pipetoterm[0]);
		dup2(pipetoshell[0],STDIN_FILENO);
        	dup2(pipetoterm[1],STDOUT_FILENO);
		dup2(pipetoterm[1],STDERR_FILENO);
		close(pipetoterm[0]);
		close(pipetoshell[1]);
		if(execvp(exec_bash[0],exec_bash) == -1)
		{
			fprintf(stderr,"Execvp error");
			exit(1);
		}
	}else if (pid > 0)//parent
	{
		close(pipetoterm[1]);
		close(pipetoshell[0]);
		parentdirect();
	}
	
	
}
int main(int argc, char *argv[])
{
	 shell_flag = 0;
	//option parsing
        struct option long_option[] = {
                {"port",required_argument , 0 , 'p'},
				{"compression", no_argument,0,'c'},
                {0,0,0,0}
        };
	
	while(1)
    {
        op = getopt_long(argc,argv, "s", long_option, 0);
        if(op ==-1)
			break;

			switch(op)
			{
				case 'c':
					comp_flag = 1;
					break;	
				case 'p':
					shell_flag = 1;
					portnum =atoi(optarg);
					signal(SIGINT,handler);
					signal(SIGPIPE, handler);
					break;
				default:
					exit(1);
					break;
					//just let it go to write;
			}
	}
	if(shell_flag)
	{
			
		if(pipe(pipetoshell) == -1 || pipe(pipetoterm) == -1)
		{
               		
			fprintf(stderr,"error in pipe creation");
			exit(1);
		}
		if((pid=fork()) >= 0)//fork process
		{
			pipeconnect();
		}else{
			fprintf(stderr,"Fork Error");
		}	exit(1);	
				
	}else
	{

    		while (1)
        	{
                	p = read (STDIN_FILENO, buffer, 256);

			for(i = 0; i < p; i++)
			{
				switch(buffer[i])
				{
					case '\004':
						exit(0);
					case '\r':
					case '\n':
						write(1,&cr,1);
                        write(1,&lf,1);
						break;
					default:
						write(1,buffer,p);			
						break;

				}
			}
		}
				
	}





	exit(0);
}















