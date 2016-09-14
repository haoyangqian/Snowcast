#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "networks.h"




void error(const char *msg){
  perror(msg);
  exit(0);
}

int send_hello(int fd,uint16_t udpPort){
    struct cmd_command hello;
    hello.commandType = 0;
    hello.content = udpPort;
    int buflen = sizeof(uint8_t)+sizeof(uint16_t);
    char buf[buflen];
    set_cmd(buf,&hello);
    if(send(fd,buf,buflen,0) < 0){
        perror("Error:send()");
        close(fd);
        exit(1);
    }
    return 0;
}

int send_setstation(int fd,uint16_t stationNumber){
    struct cmd_command setstation;
    setstation.commandType = 1;
    setstation.content = stationNumber;
    int buflen = sizeof(uint8_t)+sizeof(uint16_t);
    char buf[buflen];
    set_cmd(buf,&setstation);
    if(send(fd,buf,buflen,0) < 0){
        perror("Error:send()");
        close(fd);
        exit(1);
    }
    return 0;
}

int recv_welcome(int fd,int *n_stations){
    struct reply_welcome welcome;
    int buflen = sizeof(uint8_t)+sizeof(uint16_t);
    char buf[buflen];
    memset(buf,0,sizeof(buf));

    int bytes;

    bytes = recv(fd,buf,buflen,0);
    if(bytes < 0){
        perror("Error:recv()");
        close(fd);
        exit(1);
    }
    else if(bytes == 0){
        perror("Error:Server closed.\n");
        close(fd);
        exit(1);
    }
    get_welcome(buf,&welcome);
    *n_stations = welcome.numStations;
    return 0;
}

int recv_String(int fd,struct reply_String* string){
    int buflen = BUF_LEN_MAX;
    char buf[buflen];
    memset(buf,0,sizeof(buf));

    int bytes;

    bytes = recv(fd,buf,buflen,0);

    if(bytes < 0){
        perror("Error:recv()\n");
        close(fd);
        exit(1);
    }
    else if(bytes == 0){
        perror("Error:Server closed.\n");
        close(fd);
        exit(1);
    }
    get_String(buf,string);
    return 0;
}

int open_client(const char* hostname,int ServerPort){
    struct sockaddr_in server_addr;
    struct hostent *server;
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
        error("ERROR opening socket\n");
    server = gethostbyname(hostname);
    if(server == NULL){
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    memset((char *) &server_addr, 0 ,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy((char *)&server_addr.sin_addr.s_addr,(char *)server->h_addr,server->h_length);
    server_addr.sin_port = htons(ServerPort);
    if(connect(sockfd,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0){
        error("ERROR connecting");
    }

    printf("%s -> %s \n",hostname,inet_ntoa(server_addr.sin_addr));
    return sockfd;
}

void snowcast_control(const char* hostname,int ServerPort,int udpPort){
    int sockfd,n_stations,state;
    struct reply_String str;
    state = REP_WEL;
    sockfd = open_client(hostname,ServerPort);
    printf("Send Hello Command to the Server\n");
    send_hello(sockfd,udpPort);
    printf("Type in a number to set the station we're listening to to that number.\n");
    printf("Type in 'q' or press CTRL+C to quit.\n");

    fd_set readfd;
    struct timeval timeout;
    timeout.tv_sec = 5;
    int maxfd = sockfd > fileno(stdin) ? sockfd : fileno(stdin);


    while(1){
        FD_ZERO(&readfd);
        FD_SET(sockfd,&readfd);
        FD_SET(fileno(stdin),&readfd);
        int ret = select(maxfd+1,&readfd,NULL,NULL,&timeout);
        if(ret == -1){
            perror("Error:select error.\n");
            exit(1);
        }
        else{
            if(FD_ISSET(sockfd,&readfd)){
                if(state == REP_WEL){
                    recv_welcome(sockfd,&n_stations);
                    printf("> Receive WELCOME message.The station numbers are %d\n> ",n_stations);
                    fflush(stdout);
                }
                else if(state == REP_ANC){
                    recv_String(sockfd,&str);
                    if(str.replyType == 1){
                        printf("New song announced: %s\n> ",str.stringContent);
                        fflush(stdout);
                    }
                    else if(str.replyType == 2){
                        printf("INVALID_COMMAND_REPLY:%s\n",str.stringContent);
                        exit(1);
                    }

                }
            }
            else if(FD_ISSET(fileno(stdin),&readfd)){
                int c = fgetc(stdin);
                if(c=='q'){
                    printf("Quit.Goodbye.\n");
                    exit(0);
                }
                ungetc(c,stdin);
                char buffer[BUF_LEN_MAX];
                fgets(buffer,sizeof(buffer),stdin);
                int num;
                if(recvIntArg(&num,buffer) < 0){
                    printf("Invalid input: number or 'q' expected.\n> ");
                    fflush(stdout);
                }
                else{
                    send_setstation(sockfd,num);
                    state = REP_ANC;
                    printf("Waiting for an announce...\n");

                }            
            }
        }
    }

}

int main(int argc,char* argv[]){
    int ServerPort,udpPort;

    if(argc < 4) {
        fprintf(stderr,"Usage: ./snowcast_control servername serverport udpport\n");
        exit(0);
    }
    if(recvIntArg(&ServerPort,argv[2]) < 0){
        error("ERROR,wrong serverPort.\n");
    }
    if(recvIntArg(&udpPort,argv[3]) < 0){
        error("ERROR,wrong udpPort.\n");
    }

    snowcast_control(argv[1],ServerPort,udpPort);

    return 0;
}
