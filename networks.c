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

void set_cmd(unsigned char* buf,const struct cmd_command* cmd){
    *buf = cmd->commandType;
    uint16_t content = htons(cmd->content);
    memcpy(buf+1,&content,sizeof(uint16_t));
}

void get_welcome(const char *buf,struct reply_welcome* wel){
    wel->replyType = 0;
    uint16_t n;
    memcpy(&n,buf+1,sizeof(uint16_t));
    n= ntohs(n);
    wel->numStations = n;
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
