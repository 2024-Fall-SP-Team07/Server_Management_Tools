#ifndef NETWORK_INFO_H

#include "network_info_struct.h"

#define NETWORK_INFO_H
#define BONDING_PATH_LEN 32
#define CONTENT_LEN 1024

void get_Net_Byte(NET_Result*);
NET_Result* get_IPv4(NET_Result*, short*, short*);

#endif

