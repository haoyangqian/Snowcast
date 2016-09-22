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
#include <poll.h>
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

/*just to test the timeout situation*/
int send_timeout_hello(int fd){
    int buflen = 1;
    char buf[1];
    *buf = 0;
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

int recv_welcome(int fd,struct reply_welcome* wel){
    int buflen = sizeof(uint8_t)+sizeof(uint16_t);
    char buf[buflen];
    memset(buf,0,sizeof(buf));
    /*set timeout to 100ms*/
    int ret;
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    int total_bytes = 0;
    int bytes;

    while(total_bytes < buflen){
        bytes = recv(fd,buf+total_bytes,buflen-total_bytes,0);
        total_bytes += bytes;
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
        else if(total_bytes < buflen){
            ret = poll(&pfd,1,100);
            if(ret == 0){
                perror("Error:Timeout in recv_hello();Missing Bytes.Closing Client");
                close(fd);
                exit(1);
            }
        }

    }
    get_welcome(buf,wel);
    return 0;
}

int recv_String(int fd,struct reply_String* string){
    int buflen = BUF_LEN_MAX;
    int actual_len = buflen;
    char buf[buflen];
    memset(buf,0,sizeof(buf));

    /*set timeout to 100ms*/
    int ret;
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    int total_bytes = 0;
    int bytes;

    while(total_bytes < actual_len){
        bytes = recv(fd,buf,buflen,0);
        total_bytes += bytes;
        if(bytes < 0){
            perror("Error:recv()\n");
            close(fd);
            exit(1);
        }
        else if(bytes == 0){
            perror("Error:Server closed the connection.\n");
            close(fd);
            exit(1);
        }
        else if(total_bytes >= 2 && actual_len == BUF_LEN_MAX){
            actual_len = (int)*(buf+1) + 2;
            //printf("actual bytes:%d;totalbytes:%d\n",actual_len,total_bytes);
        }
        else if(total_bytes < actual_len){
            ret = poll(&pfd,1,100);
            if(ret == 0){
                perror("Error:Timeout in recv_hello();Missing Bytes.Closing Client");
                close(fd);
                exit(1);
            }
        }
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
    memcpy((char *)&server_addr.sin_addr.s_addr,(char *)server->h_addr_list[0],server->h_length);
    server_addr.sin_port = htons(ServerPort);
    if(connect(sockfd,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0){
        error("ERROR connecting");
    }

    printf("%s -> %s \n",hostname,inet_ntoa(server_addr.sin_addr));
    return sockfd;
}

void snowcast_control(const char* hostname,int ServerPort,int udpPort){
    int sockfd,state;
    struct reply_String str;
    struct reply_welcome wel;
    state = DEFAULT;
    sockfd = open_client(hostname,ServerPort);
    printf("Send Hello Command to the Server\n");
    //sleep(1);
    send_hello(sockfd,udpPort);
    //send_timeout_hello(sockfd);
    state = WAIT_WEL;
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
                if(state == DEFAULT){
                    perror("> Error:Receive something before send HELLO.Closing connection\n");
                    exit(1);
                }
                if(state == WAIT_WEL){
                    recv_welcome(sockfd,&wel);
                    if(wel.replyType == 0){
                        printf("> Receive WELCOME message.The station numbers are %d\n> ",wel.numStations);
                        fflush(stdout);
                    }
                    else if(wel.replyType == 1){
                        perror("Error:Receive announce before set_station.Closing connection\n");
                        exit(1);
                    }
                    else if(wel.replyType == 2){
                        printf("> INVALID_COMMAND_REPLY:%s.Closing connection\n",str.stringContent);
                        exit(1);
                    }
                    else{
                        perror("> An unknown response was received.Closing connection\n");
                        exit(1);
                    }
                }
                else if(state == WAIT_ANC){
                    recv_String(sockfd,&str);
                    if(str.replyType == 1){
                        printf("New song announced: %s\n> ",str.stringContent);
                        fflush(stdout);
                    }
                    else if(str.replyType == 0){
                        perror("> Error:receive more than one welcome.Closing connection\n");
                        exit(1);
                    }
                    else if(str.replyType == 2){
                        printf("> INVALID_COMMAND_REPLY:%s.Closing connection\n",str.stringContent);
                        exit(1);
                    }
                    else{
                        perror("> An unknown response was received.Closing connection\n");
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
                    state = WAIT_ANC;
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

