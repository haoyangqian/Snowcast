#ifndef NETWORKS_H
#define NETWORKS_H
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>


#define DEFAULT 0
#define WAIT_WEL 1
#define WAIT_ANC 2
#define RCV_ANC 3

#define BYTES_PER_SEC 16*1024
#define FREQUENCY 16
#define BUF_LEN_MAX 512

#define BYTES_PER_CMD 3

#define NANO_PER_SEC 1000000000

#define FIR_CON  0
#define RCV_WEL  1
#define SEND_SET 2
#define RCV_ANC  3

//struct for hello and setstation
struct cmd_command{
    uint8_t commandType;
    uint16_t content;
};

struct reply_welcome{
    uint8_t replyType;
    uint16_t numStations;
};

//struct for Announce and InvalidCommand
struct reply_String{
    uint8_t replyType;
    uint8_t stringSize;
    char*   stringContent;
};

struct client_info{
    int    clientfd;
    int    state;
    uint16_t udpPort;
    struct station_info* station;
    struct sockaddr_in udp_addr;
    struct sockaddr_in cli_addr;
    struct client_info* next;
};


struct station_info{
    int station_id;
    int udpfd;
    char* songname;
    FILE* song;
    struct client_info* clients;

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

void set_cmd(char* buf,const struct cmd_command* cmd);

void set_String(char *buf,struct reply_String* str);

void get_welcome(const char *buf,struct reply_welcome* wel);

void get_cmd(struct cmd_command* cmd,const char *buf);

void get_String(const char *buf,struct reply_String* str);

void time_diff(struct timespec* start,struct timespec* end,struct timespec* result);
#endif // NETWORKS_H
