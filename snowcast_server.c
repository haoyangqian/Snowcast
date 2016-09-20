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
#include <sys/syscall.h>
#include "networks.h"



/*Global variables*/
int n_staions;
struct station_info* stations;

#define DUP_HELLO   -2
#define INVALID_CMD -3
#define CLI_CLOSE   -4

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

inline void print_clientid(int clientfd){
    printf("clientid %d :",clientfd);
}

int recv_hello(int fd,struct client_info* client){
    struct cmd_command hello;
    int buflen = sizeof(uint8_t)+sizeof(uint16_t);
    char buf[buflen];
    memset(buf,0,sizeof(buf));

    int bytes;

    bytes = recv(fd,buf,buflen,0);
    if(bytes < 0){
        perror("Error:recv_hello()");
        return -1;
    }
    else if(bytes == 0){
        perror("Error:Client closed.\n");
        return -1;
    }
    get_cmd(&hello,buf);
    client->udpPort = hello.content;
    return 0;
}


int send_welcome(int fd){
    struct reply_welcome wel;
    wel.replyType = 0;
    wel.numStations =n_staions;
    int buflen = sizeof(uint8_t)+sizeof(uint16_t);
    char buf[buflen];
    set_cmd(buf,(struct cmd_command*)&wel);
    if(send(fd,buf,buflen,0) < 0){
        perror("Error:send_welcome()\n");
       return -1;
    }
    return 0;
}


int recv_setstation(int fd,int* stationid){

    struct cmd_command set_station;
    int buflen = sizeof(uint8_t)+sizeof(uint16_t);
    char buf[buflen];
    memset(buf,0,sizeof(buf));

    int bytes;

    bytes = recv(fd,buf,buflen,0);
    if(bytes < 0){
        perror("Error:recv_setstation() ");
        return -1;
    }
    else if(bytes == 0){
        return CLI_CLOSE;
    }
    get_cmd(&set_station,buf);
    *stationid = set_station.content;
    if(set_station.commandType == 0){
        return DUP_HELLO;
    }
    else if(set_station.commandType != 1 || *stationid >= n_staions || *stationid<0) return INVALID_CMD;
    return 0;
}


int send_Announce(int fd,struct client_info* client){
    struct reply_String anc;
    anc.replyType = 1;
    anc.stringSize = strlen(client->station->songname);
    anc.stringContent = client->station->songname;
    int buflen = 2*sizeof(uint8_t)+anc.stringSize;
    char buf[buflen];
    set_String(buf,&anc);
    if(send(fd,buf,buflen,0) < 0){
        perror("Error:send_Announce()\n");
        return -1;
    }
    return 0;
}

/*Send an invlid string to client*/

int send_Invalid(int fd){
    char invalid_info[128] = "server received a SET_STATION command with an invalid station number";
    struct reply_String invalid;
    invalid.replyType = 2;
    invalid.stringSize = strlen(invalid_info);
    invalid.stringContent = invalid_info;
    int buflen = 2*sizeof(uint8_t) + invalid.stringSize;
    char buf[buflen];
    set_String(buf,&invalid);
    if(send(fd,buf,buflen,0) < 0){
        perror("Error:send_Invalid()\n");
        return -1;
    }
    return 0;
}

/*Add a client to a station's list*/

int set_staion(struct client_info* client,int stationid){
    struct station_info* newstation = &stations[stationid];

    client->next = newstation->clients;
    newstation->clients=client;
    client->station = newstation;
    return 0;
}

/*remove a client from its previous station's list*/

int unset_station(struct client_info* client){
    if(client->station==NULL) return 0;
    struct station_info* station = client->station;

    struct client_info* tmp = station->clients;
    if(tmp==client) station->clients = station->clients->next;
    else{
        while(tmp->next != client && tmp!=NULL){
            tmp = tmp->next;
        }
        if(tmp==NULL){
            perror("Error:No this client in previous station.\n");
            return -1;
        }
        else tmp->next = tmp->next->next;
    }
    client->station = NULL;
    return 0;

}

/* print all stations information*/

void print_stations(){
    int i;
    for(i=0;i < n_staions;++i){
        printf("Station %d playing \"%s\", listening:",stations[i].station_id,stations[i].songname);
        struct client_info* client = stations[i].clients;
        while(client!=NULL){
            printf("%s:%d ",inet_ntoa(client->cli_addr.sin_addr),client->udpPort);
            client = client->next;
        }
        printf("\n");
    }
}


void* handle_station_thread(void* args){

}


void* handle_client_thread(void* args){
    struct client_info* client;
    client = (struct client_info *) args;
    int stationid;
    int clientfd = client->clientfd;
    print_clientid(clientfd);
    printf("new client connected;expecting HELLO\n");
    if(recv_hello(clientfd,client) < 0){
        perror("Error:receive Hello.\n");
        Close_Client(client);
    }
    if(send_welcome(clientfd)<0){
        perror("Error:send Welcome.\n");
    }
    print_clientid(clientfd);
    printf("HELLO received;sending WELCOME, expecting SET_STATION\n");

    while(1){
        int ret;
        if((ret = recv_setstation(clientfd,&stationid)) < 0){
            if(ret == DUP_HELLO){
                print_clientid(clientfd);
                perror("Error:receive duplicate HELLO.\n");
                Close_Client(client);
            }
            else if(ret == INVALID_CMD){
                print_clientid(clientfd);
                printf("received request for invalid station,sending INVALID_COMMAND;closing connection\n");
                if(send_Invalid(clientfd)<0){
                    print_clientid(clientfd);
                    perror("Error:send Invalid.\n");
                    exit(1);
                }
                Close_Client(client);
            }
            else if(ret == CLI_CLOSE){
                print_clientid(clientfd);
                printf("Client close connection\n");
                Close_Client(client);
            }else{
                print_clientid(clientfd);
                perror("Error:receive SetStation.\n");
                Close_Client(client);
            }
        }
        print_clientid(clientfd);
        printf("received SET_STATION to station %d\n",stationid);
        if(unset_station(client) < 0){
            print_clientid(clientfd);
            perror("Error:unset Station.\n");
            Close_Client(client);
        }
        if(set_staion(client,stationid) < 0){
            print_clientid(clientfd);
            perror("Error:set Station.\n");
            Close_Client(client);
        }

        if(send_Announce(clientfd,client) < 0){
            print_clientid(clientfd);
            perror("Error:send Announce.\n");
        }
    }
    return 0;
}


void accpet_client(int clientfd,struct sockaddr_in cli_addr){
    struct client_info* cl = malloc(sizeof(struct client_info));
    cl->clientfd = clientfd;
    cl->cli_addr = cli_addr;
    cl->udp_addr = cli_addr;
    cl->station  = NULL;
    cl->next = NULL;
    pthread_t thread;
    pthread_create(&thread, NULL, handle_client_thread,(void *)cl);
    pthread_detach(thread);
}

int open_udp(){
    int fd;
    if((fd = socket(AF_INET,SOCK_DGRAM,0)) < 0){
        perror("Error:open_udp()");
        exit(1);
    }
    return fd;
}


void Close_Client(struct client_info* client){
    print_clientid(client->clientfd);
    printf("Close client connection\n");
    unset_station(client);
    close(client->clientfd);
    free(client);
    pthread_exit(NULL);
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
    struct sockaddr_in cli_addr;
    socklen_t clilen;
    serverfd = open_Server(serverPort);
    fd_set readfd;
    int maxfd = serverfd > fileno(stdin) ? serverfd : fileno(stdin);
    while(1){
        FD_ZERO(&readfd);
        FD_SET(serverfd,&readfd);
        FD_SET(fileno(stdin),&readfd);
        int ret = select(maxfd+1,&readfd,NULL,NULL,NULL);
        if(ret == -1){
            perror("Error:select error.\n");
            exit(1);
        }
        else{
            if(FD_ISSET(serverfd,&readfd)){
                if((newsockfd = accept(serverfd,(struct sockaddr *) &cli_addr,&clilen))<0){
                    printf("Error on accept.\n");
                }
                accpet_client(newsockfd,cli_addr);
            }
            else if(FD_ISSET(fileno(stdin),&readfd)){
                int c = fgetc(stdin);
                if(c!='\n'){
                    if(c=='q'){
                        printf("Quit.Goodbye.\n");
                        exit(0);
                    }
                    else if(c=='p'){
                        print_stations();
                    }
                    else{
                        printf("Invalid input: 'p'or 'q' expected.\n ");
                    }
                }
            }
        }
    }

}


int main(int argc,char *argv[]){
    int serverPort;
    n_staions = argc -2;
    struct song_table song_t;
    stations = malloc(n_staions*sizeof(struct station_info));
    if(argc < 3) {
        fprintf(stderr,"Usage: ./snowcast_server serverport songame\n");
        exit(0);
    }

    if(recvIntArg(&serverPort,argv[1])< 0 ){
       error("ERROR,wrong serverPort.\n");
    }
    int i = 2;
    for(i = 2;i < argc;++i){
        stations[i-2].songname = argv[i];
        stations[i-2].station_id = i-2;
        stations[i-2].song = NULL;
        stations[i-2].udpfd = open_udp();
        stations[i-2].clients = NULL;
        printf("songname:%s\n",stations[i-2].songname);
    }
    snowcast_server(serverPort);
    return 0;
}


