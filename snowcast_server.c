#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <poll.h>
#include "networks.h"



/*Global variables*/
int n_staions;
struct station_info* stations;
pthread_mutex_t* mutex_stations;

#define DUP_HELLO        -2
#define INVALID_CMD      -3
#define CLI_CLOSE        -4
#define WRONG_STATIONNUM -5


/*function declearation*/
int recv_hello(int fd,struct client_info* client);

int send_welcome(int fd);

int recv_setstation(struct client_info* client,int* stationid);

int send_Announce(struct client_info* client);

int send_Invalid(int fd,char* invalid_info);

int set_staion(struct client_info* client,int stationid);

int unset_station(struct client_info* client);

void print_stations();

void time_diff(struct timespec* start,struct timespec* end,struct timespec* result);

int stream_music(struct station_info* station);

void* handle_station_thread(void* args);

void* handle_client_thread(void* args);

void accpet_client(int clientfd,struct sockaddr_in cli_addr);

int open_udp();

void Close_Client(struct client_info* client);

int open_Server(uint16_t serverPort);

void snowcast_server(uint16_t serverPort);


/*function implementation*/

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
    /*set timeout to 100ms*/
    int ret;
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    ret = poll(&pfd,1,100);
    if(ret == -1){
        perror("Error:poll()\n");
    }
    else if(ret == 0){
        perror("Error:Timeout in recv_hello();Closing Client");
        Close_Client(client);
    }
    else{
        int total_bytes = 0;
        int bytes;
        while(total_bytes < buflen){
            bytes = recv(fd,buf + total_bytes,buflen - total_bytes,0);
            total_bytes += bytes;
            if(bytes < 0){
                printf("Error:recv_hello(),bytes:%d",bytes);
                return -1;
            }
            else if(bytes == 0){
                perror("Error:Client closed.\n");
                return -1;
            }
            else if(total_bytes < buflen){
                ret = poll(&pfd,1,100);
                if(ret == 0){
                    perror("Error:Timeout in recv_hello();Missing Bytes.Closing Client");
                    Close_Client(client);
                }
            }

        }
        get_cmd(&hello,buf);
        if(hello.commandType!=0){
            return INVALID_CMD;
        }
        client->udpPort = hello.content;
        client->udp_addr.sin_port = htons(client->udpPort);
    }
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

int recv_setstation(struct client_info* client,int* stationid){

    struct cmd_command set_station;
    int buflen = sizeof(uint8_t)+sizeof(uint16_t);
    char buf[buflen];
    memset(buf,0,sizeof(buf));

    /*set timeout to 100ms*/
    int ret;
    struct pollfd pfd;
    pfd.fd = client->clientfd;
    pfd.events = POLLIN;
    int total_bytes = 0;
    int bytes;

    while(total_bytes < buflen){
        bytes = recv(client->clientfd,buf+total_bytes,buflen-total_bytes,0);
        total_bytes+=bytes;
        if(bytes < 0){
            perror("Error:recv_setstation() ");
            return -1;
        }
        else if(bytes == 0){
            return CLI_CLOSE;
        }
        else if(total_bytes < buflen){
            ret = poll(&pfd,1,100);
            if(ret == 0){
                perror("Error:Timeout in recv_setstation();Missing Bytes.Closing Client");
                Close_Client(client);
            }
        }

    }

    get_cmd(&set_station,buf);
    *stationid = set_station.content;
    if(set_station.commandType == 0){
        return DUP_HELLO;
    }
    else if(*stationid >= n_staions || *stationid<0) return WRONG_STATIONNUM;
    else if(set_station.commandType != 1)return INVALID_CMD;

    return 0;
}

int send_Announce(struct client_info* client){
    struct reply_String anc;
    anc.replyType = 1;
    anc.stringSize = strlen(client->station->songname);
    anc.stringContent = client->station->songname;
    int buflen = 2*sizeof(uint8_t)+anc.stringSize;
    char buf[buflen];
    set_String(buf,&anc);
    if(send(client->clientfd,buf,buflen,0) < 0){
        perror("Error:send_Announce()\n");
        return -1;
    }
    return 0;
}

/*Send an invlid string to client*/
int send_Invalid(int fd,char* invalid_info){
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
    pthread_mutex_lock(&mutex_stations[stationid]);
    struct station_info* newstation = &stations[stationid];
    client->next = newstation->clients;
    newstation->clients=client;
    client->station = newstation;
    pthread_mutex_unlock(&mutex_stations[stationid]);
    return 0;
}

/*remove a client from its previous station's list*/
int unset_station(struct client_info* client){
    if(client->station==NULL) return 0;
    struct station_info* station = client->station;
    int stationid = station->station_id;
    pthread_mutex_lock(&mutex_stations[stationid]);
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
    pthread_mutex_unlock(&mutex_stations[stationid]);
    return 0;

}

/* print all stations information*/
void print_stations(){
    int i;
    for(i=0;i < n_staions;++i){
        printf("Station %d playing \"%s\", listening:",
               stations[i].station_id,stations[i].songname);
        struct client_info* client = stations[i].clients;
        while(client!=NULL){
            printf("%s:%d ",inet_ntoa(client->cli_addr.sin_addr),client->udpPort);
            client = client->next;
        }
        printf("\n");
    }
}



int stream_music(struct station_info* station){
    FILE *fp;
    int buflen = BYTES_PER_SEC/FREQUENCY;
    char* buffer = malloc(buflen * sizeof(char));
    struct client_info* client;
    struct timespec timeval,start,end;
    fp = fopen(station->songname,"r");
    if(fp==NULL){
        printf("ERROR:open song file.\n");
        return -1;
    }
    station->song = fp;
    while(1){
        timeval.tv_sec = 0;
        timeval.tv_nsec = NANO_PER_SEC/FREQUENCY;
        clock_gettime(CLOCK_REALTIME,&start);
        int bytes;
        if((bytes = fread(buffer,1,buflen,fp))<0){
            perror("Error:Read files()");
        }
        int sent_bytes = bytes;
        while(sent_bytes <= buflen){
            if(feof(fp)){
               rewind(fp);
               /*send Announce Again*/
               pthread_mutex_lock(&mutex_stations[station->station_id]);
               client = station->clients;
               while(client!=NULL){
                   if(send_Announce(client)<0){
                       perror("Error:send Announce()");
                   }
                   client = client->next;
               }
               pthread_mutex_unlock(&mutex_stations[station->station_id]);
               if((bytes = fread(buffer+sent_bytes,1,buflen-sent_bytes,fp))<0){
                   perror("Error:Read files(),in eof loop");
               }
               sent_bytes += bytes;
            }
            else break;
        }
        pthread_mutex_lock(&mutex_stations[station->station_id]);
        client = station->clients;
        while(client!=NULL){
            if((bytes = sendto(station->udpfd,buffer,buflen,0,
                  (struct sockaddr*)&client->udp_addr,sizeof(client->udp_addr)))<0){
                perror("Error:sendto error.\n");
            }
            client = client->next;
        }
        pthread_mutex_unlock(&mutex_stations[station->station_id]);
        clock_gettime(CLOCK_REALTIME,&end);
        struct timespec tmp;
        time_diff(&start,&end,&tmp);
        time_diff(&tmp,&timeval,&timeval);
        nanosleep(&timeval,NULL);
    }
}

void* handle_station_thread(void* args){
    struct station_info* station;
    station = (struct station_info*) args;
    stream_music(station);
    return 0;
}



void* handle_client_thread(void* args){
    struct client_info* client;
    client = (struct client_info *) args;
    int stationid;
    int clientfd = client->clientfd;
    print_clientid(clientfd);
    printf("new client connected;expecting HELLO\n");
    int ret;
    if((ret = recv_hello(clientfd,client)) < 0){
        /* if any cmd is received before hello,close the connection*/
        if(ret == INVALID_CMD){
            perror("Error:receive INVALID_CMD before HELLO");
            Close_Client(client);
        }
        else{
            perror("Error:receive Hello.\n");
            Close_Client(client);
        }
    }
    if(send_welcome(clientfd)<0){
        perror("Error:send Welcome.\n");
    }
    print_clientid(clientfd);
    printf("HELLO received;sending WELCOME, expecting SET_STATION\n");
    client->state = RCV_WEL;
    while(1){
        int ret;
        if((ret = recv_setstation(client,&stationid)) < 0){
            if(ret == DUP_HELLO){
                char invalid_info[128] = "server received more than one HELLO";
                print_clientid(clientfd);
                printf("Error:%s;closing connection\n",invalid_info);
                if(send_Invalid(clientfd,invalid_info)<0){
                    print_clientid(clientfd);
                    perror("Error:send Invalid.\n");
                    exit(1);
                }
                Close_Client(client);
            }
            else if(ret == WRONG_STATIONNUM){
                print_clientid(clientfd);
                char invalid_info[128] = "server received a SET_STATION command with an invalid station number";
                printf("Error:%s;closing connection\n",invalid_info);
                if(send_Invalid(clientfd,invalid_info)<0){
                    print_clientid(clientfd);
                    perror("Error:send Invalid.\n");
                    exit(1);
                }
                Close_Client(client);
            }
            else if(ret == INVALID_CMD){
                print_clientid(clientfd);
                char invalid_info[128] = "server received a INVALID_COMMAND";
                printf("Error:%s;closing connection\n",invalid_info);
                if(send_Invalid(clientfd,invalid_info)<0){
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
        client->state = SEND_SET;
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

        if(send_Announce(client) < 0){
            print_clientid(clientfd);
            perror("Error:send Announce.\n");
        }
        client->state = RCV_ANC;
    }
    return 0;
}


void accpet_client(int clientfd,struct sockaddr_in cli_addr){
    struct client_info* cl = malloc(sizeof(struct client_info));
    cl->clientfd = clientfd;
    cl->cli_addr = cli_addr;
    cl->udp_addr = cli_addr;
    cl->state = FIR_CON;
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
    int enable = 1;
    //Allow server to reuse its port.
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,&enable, sizeof(int)) < 0)
        error("setsockopt(SO_REUSEADDR) failed");

    /*
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,(char*)&tv,sizeof(struct timeval));*/
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

void Close_Server(){
    int i;
    for(i = 0;i < n_staions;++i){
        struct client_info* client;
        struct client_info* tmp;
        client = stations->clients;
        while(client){
            tmp = client->next;
            close(client->clientfd);
            free(client);
            client = tmp;
        }
    }
    free(stations);
    free(mutex_stations);
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
                        Close_Server();
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
    stations = malloc(n_staions*sizeof(struct station_info));
    mutex_stations = malloc(n_staions*sizeof(pthread_mutex_t));
    pthread_t thread[n_staions];
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
         pthread_mutex_init(&mutex_stations[i-2],NULL);
        pthread_create(&thread[i-2], NULL, handle_station_thread,(void *)&stations[i-2]);
        printf("songname:%s\n",stations[i-2].songname);
    }
    snowcast_server(serverPort);
    return 0;
}


