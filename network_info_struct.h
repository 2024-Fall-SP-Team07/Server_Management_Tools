#ifndef NETWORK_INFO_STRUCT_H

#include <net/if.h>

#define NETWORK_INFO_STRUCT_H
#define IPV4_LEN 16

typedef struct NET_Result {
    unsigned long receive_byte[2];
    unsigned long transmit_byte[2];
    unsigned long receive_error;
    unsigned long transmit_error;
    unsigned long receive_drop;
    unsigned long transmit_drop;
    struct NET_Result *next;
    char ipv4_addr[IPV4_LEN];
    char ifa_name[IFNAMSIZ];
} NET_Result;


#endif