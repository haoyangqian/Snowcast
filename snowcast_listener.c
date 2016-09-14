#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include "networks.h"

/*
int open_client(uint16_t udpPort){
    struct sockaddr_in myaddr;
    int sockfd;
    if((sockfd = socket(AF_INET,SOCK_DGRAM,0)) < 0){
        perror("cannot create socket");
        return -1;
    }

    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(udpPort);

    if(bind(sockfd,(struct sockaddr*) &myaddr,sizeof(myaddr)) < 0){
        perror("bind failed");
        return -1;
    }
    return sockfd;
}

void snowcast_listener(uint16_t udpPort){
    int clientfd;
    if((clientfd = open_client(udpPort)) < 0){
        perror("Error:Fail to open client socket");
    }

    struct sockaddr_in servadd;
    socklen_t servadd_len;

    int buflen = BYTES_TO_RECV;
    char buffer[buflen];


    while(1){
           int bytes;
           bytes = recvfrom(clientfd,buffer,buflen,0,(struct sockaddr*) &servadd,&servadd_len);

           if(bytes < 0){
               perror("Error:recv from server");
               close(clientfd);
               exit(1);
           }
           if(bytes == 0){
               fprintf(stderr,"Server close the connection");
           }
           if(write(fileno(stdout),buffer,sizeof(buffer)) < 0){
               perror("Error:writing to stdout");
               close(clientfd);
               exit(1);
           }

    }

}

int main(int argc,char* argv[]){
    if(argc!=2){
        perror("Usage: ./snowcast_listener udpport\n");
        exit(0);
    }
    int udpPort;
    if(recvIntArg(&udpPort,argv[1]) < 0){
        perror("ERROR,wrong udpPort.\n");
    }

    if(udpPort > 65535 || udpPort < 2000){
        fprintf(stderr,"udpPort is out of range");
        exit(1);
    }

    snowcast_listener(udpPort);
    return 0;
}
*/
