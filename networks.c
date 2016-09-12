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
    memcpy(&n,buf+1,sizeof(u_int16_t));
    n= ntohs(n);
    wel->numStations = n;
}

void get_announce(const char *buf,struct reply_Announce* anc){
    anc->replyType = 1;
    int songnameSize;
    char songname[songnameSize];
    memcpy(&songnameSize,buf+1,sizeof(u_int16_t));
    memcpy(&songname,buf+3,songnameSize);
    anc->songnameSize = songnameSize;
    anc->songname = &songname;
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
