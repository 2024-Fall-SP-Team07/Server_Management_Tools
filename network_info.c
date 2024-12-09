#include "network_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>

void get_Net_Byte(NET_Result* net_info){
    FILE *fp = NULL;
    NET_Result* cur = NULL;
    char buf[CONTENT_LEN] = { '\0' }, device_buf[IFNAMSIZ] = { '\0' };
    unsigned long receive_byte_buf = 0, transmit_byte_buf = 0, receive_error_buf = 0, transmit_error_buf = 0;
    unsigned long receive_drop_buf = 0, transmit_drop_buf = 0;
    if ((fp = fopen("/proc/net/dev", "r")) == NULL) {
        return;
    }
    while (fgets(buf, sizeof(buf), fp) != NULL){
        cur = net_info;
        sscanf(buf, "%s %ld %*d %ld %ld %*d %*d %*d %*d %ld %*d %ld %ld %*d %*d %*d %*d", device_buf, &receive_byte_buf, &receive_error_buf, &receive_drop_buf,
        &transmit_byte_buf, &transmit_error_buf, &transmit_drop_buf);
        device_buf[strlen(device_buf) - 1] = '\0';
        while (cur != NULL){
            if (strcmp(device_buf, cur->ifa_name) == 0) {
                cur->receive_byte[1] = receive_byte_buf; 
                cur->receive_error = receive_error_buf;
                cur->receive_drop = receive_drop_buf;
                cur->transmit_byte[1] = transmit_byte_buf;
                cur->transmit_error = transmit_error_buf;
                cur->transmit_drop = transmit_drop_buf;
                break;
            }
            cur = cur->next;
        }
    }
    fclose(fp);
}

NET_Result* get_IPv4(NET_Result* priv_data, short* max_length, short* ifa_count){
    struct ifaddrs *ifaddr, *ifa;
    NET_Result *head = NULL, *node = NULL, *cur = NULL, *priv_cur = priv_data, *free_node;
    char ip[IPV4_LEN];
    short len = 0, count = 0;
    if (getifaddrs(&ifaddr) == -1) {
        return NULL;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){
        if (ifa->ifa_addr == NULL){
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4 인 경우 출력
            if ((node = (NET_Result*)calloc(1, sizeof(NET_Result))) == NULL) {
                return NULL;
            }
            len = ((short)strlen(ifa->ifa_name) > len) ? (short)strlen(ifa->ifa_name) : len;
            struct sockaddr_in *addr = (struct sockaddr_in *) ifa -> ifa_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
            strcpy(node->ifa_name, ifa->ifa_name);
            strcpy(node->ipv4_addr, ip);
            if (priv_cur != NULL){
                node->receive_byte[0] = priv_cur->receive_byte[1];
                node->transmit_byte[0] = priv_cur->transmit_byte[1];
                free_node = priv_cur;
                priv_cur = priv_cur->next;
                free(free_node);
            }
            node->next = NULL;
            if (head == NULL){
                head = node;
                cur = node;
            } else {
                cur->next = node;
                cur = node;
            }
            count++;
        }
    }
    (*max_length) = len;
    (*ifa_count) = count;
    return head;
}