//NAME: HARRY YEE




#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include "zlib.h"
#include <stdlib.h>


//termios and poll
struct termios saved_settings;
struct pollfd pollKeyServer[2];
const char cr=0xD;
const char lf=0xA;
char buffer[4000];
int p;
int op;
int i=0;
int log_flag = 0;
int comp_flag = 0;
char* logs = NULL;
int logfd = -1;
//variables for comp/decomp
z_stream ClientToServer;
z_stream ServerToClient;
char inbuffer[4000] = "hello";
char outbuffer[4000];


//variables for socket
int sockfd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;
void logwrite(int bsize, char * buf,int sent)
{
	char c[2];
	sprintf(c, "%d", bsize);
	if(sent)//for sending out
	{
		write(logfd,"SENT ",5);
	}else
	{
		write(logfd,"RECEIVED ",9);
	}
	    write(logfd,&c,2);
        write(logfd," bytes: ",8);
        write(logfd,buf,bsize);
        write(logfd,"\n",1);

}
void restore_term()
{   
    tcsetattr (STDIN_FILENO, TCSANOW, &saved_settings);
}

void init_term()
{
    struct termios tattr;
    if (!isatty (STDIN_FILENO))
    {
            fprintf (stderr, "Not a terminal.\n");
            exit (1);
    }

    tcgetattr (STDIN_FILENO, &saved_settings);
    atexit (restore_term);

    tcgetattr(STDIN_FILENO, &tattr);
    tattr.c_iflag = ISTRIP;
    tattr.c_oflag = 0;
    tattr.c_lflag = 0;
    tcsetattr (STDIN_FILENO, TCSANOW, &tattr);
}
int  decomp(int read_in, char* inbuf, char* outbuf)
{

    ServerToClient.zalloc=Z_NULL;
    ServerToClient.zfree=Z_NULL;
    ServerToClient.opaque=Z_NULL;

    inflateInit(&ServerToClient);

    ServerToClient.avail_in= (uInt) read_in;
    ServerToClient.next_in = (Bytef*) inbuf;
    ServerToClient.avail_out = (uInt) 4000;
    ServerToClient.next_out = (Bytef*) outbuf;

    memset(outbuf,0,4000);
    do{
       // printf("%d\n",ServerToClient.avail_in);
        inflate(&ServerToClient,Z_SYNC_FLUSH);
    }while(ServerToClient.avail_in > 0);

    return 4000 - ServerToClient.avail_out;

    inflateEnd(&ServerToClient);
}
int comp(int read_in, char* inb, char* outb )
{   
    ClientToServer.zalloc=Z_NULL;
    ClientToServer.zfree=Z_NULL;
    ClientToServer.opaque=Z_NULL;
    
    deflateInit(&ClientToServer, Z_DEFAULT_COMPRESSION);

    
    ClientToServer.avail_in= (uInt) read_in;
    ClientToServer.next_in = (Bytef*) inb;
    ClientToServer.avail_out = (uInt) 4000;
    ClientToServer.next_out = (Bytef*) outb;
    
    memset(outb,0,4000);
    
    do{ 
        deflate(&ClientToServer,Z_SYNC_FLUSH);
    }while(ClientToServer.avail_in > 0);
	
	return 4000 - ClientToServer.avail_out;
	deflateEnd(&ClientToServer);	
}

void socketinit(int portnum)
{
    portno = portnum;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        fprintf(stderr,"ERROR opening socket");
		exit(0);
	}
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    memset((char *) &serv_addr,0 ,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        fprintf(stderr,"ERROR connecting");

}
void runPoll()
{
	int sendR = 0;
	pollKeyServer[0].fd = STDIN_FILENO;
    pollKeyServer[1].fd = sockfd;//socket to server;
    pollKeyServer[0].events = POLLIN | POLLHUP | POLLERR;
    pollKeyServer[1].events = POLLIN | POLLHUP | POLLERR | POLLRDHUP;
    char  buf2[4000];
	char buf3[4000];
	char buf4[4000];
	memset(buf3,0,4000);
	//int j = 0;
    while(1)
    {
		int p1 = poll(pollKeyServer,2,0);
		if(p1 == -1)
		{
			exit(0);
		}	
		if(pollKeyServer[0].revents & POLLIN)
        {
            p = read (STDIN_FILENO, buf2, 4000);
			//memset(buf4,0,4000);
			sendR= 1;
            /*for(i = 0; i < p; i++)
			{

                switch(buf2[i])
                {
                    case '\004':
                        write(1,"^D",2);
                        exit(1);
                        break;
                    case '\003':
                        write(1,"^C",2);
						//write(sockfd,"\003",1);
						buf4[j] = '\003';
						j++;
                        //exit(1);
                        break;
                    case '\r':
                    case '\n':
						write(1,&cr,1);
						write(1,&lf,1);
						buf4[j] = buf2[i];
						j++;
						//sendR=1;
						break;
                    default:
						write(1,buf2 + i,1);//write buff to display
						buf4[j] = buf2[i];
						j++;
                        break;
                }
			}*/
		}	
		if(sendR)
		{
			//if(log_flag)
		//	{
		//		logwrite(j,buf4,1);
		//	}
			if(comp_flag)
			{
				int z = comp(p,buf2,buf3);
				write(sockfd,buf3,z);
				if(log_flag)
					logwrite(z,buf3,1);
			}else
			{
				if(log_flag)
					logwrite(p,buf2,1);
				write(sockfd,buf2,p);
			}
			sendR=0;
	//		j=0;
			memset(buf4,0,4000);
		}
        //shell has output to read
        if(pollKeyServer[1].revents & POLLIN)
        {
			int z;
			if(comp_flag)
			{
				p = read(sockfd,buf2,4000);
				logwrite(p,buf2,0);
				z = decomp(p,buf2,buf3);
			}else
			{
				z = read(sockfd,buf3,4000);
				logwrite(z,buf3,0);
			}
            for(i = 0; i < z; i++)
            {
                if(buf3[i] == lf)
                {
                    write(1,&cr,1);
                    write(1,&lf,1);
                }else
                {
                    write(1,buf3 + i,1);
                }
            }

        }
        //check for any errors from the sehll
		if(pollKeyServer[1].revents & (POLLHUP | POLLERR | POLLRDHUP ))
        {
			fprintf(stderr,"Exit");
			 exit(0);
        }
    }
}
int main(int argc, char *argv[])
{
    struct option long_option[] = {
		{"port", required_argument, 0 , 'p'},
		{"log", required_argument, 0, 'l'},
		{"compression", no_argument,0,'c'},
        {0,0,0,0}
    };

//	init_term();
	int portnum=0;
	int port_flag=0;
	while(1)
	{
		op = getopt_long(argc,argv, "p:c:l", long_option, 0);
		if(op ==-1)
			break;

		switch(op)
		{
			case 'l':
				log_flag = 1;
				logs = optarg;
				break;
			case 'c':
				comp_flag = 1;
				break;
			case 'p':
				port_flag = 1;
				portnum =atoi(optarg);
				break;
            default:
				exit(1);
                //just let it go to write;
        }
 
    }
	if(log_flag)
	{
		logfd = open(logs, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if(logfd < 0)
		{
			fprintf(stderr,"Error Openning");
			exit(1);
		}
	}
	 init_term();
	if(port_flag)
	{
		socketinit(portnum);
        runPoll();
	}

	

	exit(0);





}




