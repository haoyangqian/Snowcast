#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "networks.h"

void set_cmd(char* buf,const struct cmd_command* cmd){
    *buf = cmd->commandType;
    uint16_t content = htons(cmd->content);
    memcpy(buf+1,&content,sizeof(uint16_t));
}

void set_String(char *buf,struct reply_String* str){
    *buf = str->replyType;
    *(buf+1) = str->stringSize;
    memcpy(buf+2,str->stringContent,str->stringSize);
}

void get_welcome(const char *buf,struct reply_welcome* wel){
    wel->replyType = 0;
    uint16_t n;
    memcpy(&n,buf+1,sizeof(uint16_t));
    n= ntohs(n);
    wel->numStations = n;
}

void get_cmd(struct cmd_command* cmd,const char *buf){
    memcpy(&cmd->commandType,buf,sizeof(uint8_t));
    uint16_t n;
    memcpy(&n,buf+1,sizeof(uint16_t));
    n= ntohs(n);
    cmd->content = n;
}

void get_String(const char *buf,struct reply_String* str){
    memcpy(&str->replyType,buf,sizeof(uint8_t));
    memcpy(&str->stringSize,buf+1,sizeof(uint8_t));
    str->stringContent = (char*) malloc(str->stringSize+1);
    memcpy(str->stringContent,buf+2,str->stringSize);
    str->stringContent[str->stringSize]='\0';
}

int recvIntArg(int *num,char* str){
    char* end;
    *num = strtol(str,&end,10);
    //No digit in str or the number overrange the max integer of long int
    if(end == str || errno == ERANGE){
        return -1;
    }
    return 0;
}

void time_diff(struct timespec* start,struct timespec* end,struct timespec* result){
    if((end->tv_nsec - start->tv_nsec) < 0){
        result->tv_sec = end->tv_sec - start->tv_sec - 1;
        result->tv_nsec = end->tv_nsec - start->tv_nsec + NANO_PER_SEC;
    }else{
        result->tv_sec = end->tv_sec - start->tv_sec;
        result->tv_nsec = end->tv_nsec - start->tv_nsec;
    }
}
