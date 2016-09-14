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
#include <pthread.h>
#include "networks.h"

/*
void error(const char *msg)
{
    perror(msg);
    exit(1);
}



int recv_hello(int fd,uint16_t *udpPort){
    struct cmd_command hello;
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
        perror("Error:Client closed.\n");
        close(fd);
        exit(1);
    }
    get_hello(buf,&hello);
    *udpPort = hello.content;
    printf("udpPort:%d\n",*udpPort);
    return 0;
}

void* handle_thread(void* args){
    uint16_t udpPort;
    int* newsockfd = (int *) args;
    recv_hello(*newsockfd,&udpPort);
    return 0;
}

int open_Server(uint16_t serverPort){
    struct sockaddr_in serv_addr;
    int sockfd;
    if((sockfd = socket(AF_INET,SOCK_STREAM,0))<0){
        error("Error opening socket.\n");
    }
    memset((char*) &serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(serverPort);
    if(bind(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        error("Error bingding port.\n");
    }
    listen(sockfd,5);

    return sockfd;
}

void snowcast_server(uint16_t serverPort){
    int serverfd,newsockfd;
    uint16_t udpPort;
    struct sockaddr_in cli_addr;
    socklen_t clilen;
    serverfd = open_Server(serverPort);
    while(1){
        if((newsockfd = accept(serverfd,(struct sockaddr *) &cli_addr,&clilen))<0){
            printf("Error on accept.\n");
        }

        pthread_t thread;
        pthread_create(&thread, NULL, handle_thread,newsockfd);
        pthread_detach(thread);

    }

}

int main(int argc,char *argv[]){
    int serverPort;
    char* songname[10];
    if(argc < 3) {
        fprintf(stderr,"Usage: ./snowcast_server serverport songame\n");
        exit(0);
    }

    if(recvIntArg(&serverPort,argv[1])< 0 ){
       error("ERROR,wrong serverPort.\n");
    }
    int i = 2;
    for(i = 2;i < argc;++i){
        songname[i-2] = argv[i];
        printf("songname:%s\n",songname[i-2]);
    }

    snowcast_server(serverPort);
    return 0;
}
*/
