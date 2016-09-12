#ifndef NETWORKS_H
#define NETWORKS_H
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>


#define REP_WEL 1
#define REP_ANC 2

#define BYTES_TO_RECV 1024
#define BUF_LEN_MAX 512

struct cmd_command{
    uint8_t commandType;
    uint16_t content;
};

struct cmd_hello{
    uint8_t   commandType;
    uint16_t  udpPort;
};

struct cmd_setstaion{
    uint8_t   commandType;
    uint16_t  stationNumber;
};

struct reply_welcome{
    uint8_t replyType;
    uint16_t numStations;
};

struct reply_Announce{
    uint8_t replyType;
    uint8_t songnameSize;
    char*    songname;
};

struct reply_InvalidCommand{
    uint8_t replyType;
    uint8_t replyStringSize;
    char*    replyString;
};

// struct hostent {
//     char    *h_name;        /* official name of host */
//     char    **h_aliases;    /* alias list */
//     int     h_addrtype;     /* host address type */
//     int     h_length;       /* length of address */
//     char    **h_addr_list;  /* list of addresses */
// }

// struct sockaddr_in{
//   short   sin_family; /* must be AF_INET */
//   u_short sin_port;
//   struct  in_addr sin_addr;
//   char    sin_zero[8]; /* Not used, must be zero */
// };

// struct in_addr {
//     uint32_t       s_addr;     /* address in network byte order */
// };

int recvIntArg(int *num,char* str);

void set_cmd(unsigned char* buf,const struct cmd_command* cmd);

void get_welcome(const char *buf,struct reply_welcome* wel);

void get_announce(const char *buf,struct reply_Announce* anc);

#endif // NETWORKS_H
