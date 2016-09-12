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
#include <netdb.h>
#include "networks.h"




void error(const char *msg){
  perror(msg);
  exit(0);
}

int send_command(int fd,struct cmd_command* cmd){
    int buflen = sizeof(uint8_t)+sizeof(uint16_t);
    char buf[buflen];
    set_cmd(buf,cmd);

    if(send(fd,buf,buflen,0) < 0){
        perror("Error:send()");
        close(fd);
        exit(1);
    }
    return 0;
}

int send_hello(int fd,uint16_t udpPort){
    struct cmd_hello hello;
    hello.commandType = 0;
    hello.udpPort = udpPort;
    return send_command(fd,(const struct cmd_command*) &hello);
}

int send_setsation(int fd,uint16_t stationNumber){
    struct cmd_setstaion setstation;
    setstation.commandType = 1;
    setstation.stationNumber = stationNumber;
    return send_command(fd,(const struct cmd_command*) &setstation);
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
    get_welcome(buf,&welcome);
    *n_stations = welcome.numStations;
    return 0;
}

int recv_announce(int fd,char *songname){
    struct reply_Announce anc;
    int buflen = BUF_LEN_MAX;
    char buf[buflen];
    memset(buf,0,sizeof(buf));

    int bytes;

    bytes = recv(fd,buf,buflen,0);

    if(bytes < 0){
        perror("Error:recv()");
        close(fd);
        exit(1);
    }
    get_announce(buf,&anc);
    songname = anc.songname;

    return 0;
}


int open_client(char* hostname,int ServerPort){
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
    return sockfd;
}

void snowcast_control(const char* hostname,int ServerPort,int udpPort){
    int sockfd,n_stations,state;
    state = REP_WEL;
    sockfd = open_client(hostname,ServerPort);
    printf("Send Hello Command to the Server\n");
    send_hello(sockfd,udpPort);
    printf("Type in a number to set the station we're listening to to that number.\n");
    printf("Type in 'q' or press CTRL+C to quit.\n");

    fd_set readfd;
    struct timeval timeout;
    int maxfd = sockfd;
    if(fileno(stdin) > maxfd) maxfd = fileno(stdin);


    while(1){
        FD_ZERO(&readfd);
        FD_SET(sockfd,&readfd);
        FD_SET(fileno(stdin),&readfd);
        int ret = select(maxfd+1,&readfd,NULL,NULL,NULL);
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
                    state = REP_ANC;
                }
                else if(state == REP_ANC){
                    char *songname;
                    recv_announce(sockfd,songname);
                    printf("New song announced: %s",songname);
                }
            }
            else if(FD_ISSET(fileno(stdin),&readfd)){
                char c;
                read(fileno(stdin),&c,1);
                if(c=='q'){
                    printf("Quit.Goodbye.\n");
                }
                else if(c=='0'){
                    if((c-'0') >= n_stations || (c-'0') < 0){
                        perror("Error:Wrong station number.Should be 0 - (stations-1)\n");
                    }
                    else{
                        send_setsation(sockfd,c-'0');
                        printf("Waiting for an announce...\n");
                    }
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




