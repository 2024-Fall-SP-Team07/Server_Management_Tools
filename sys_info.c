#include "common.h"
#include "sys_info.h"
#include <stdio.h>
#include <string.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/sysinfo.h>

void get_Hostname(char* buf){
    FILE* fp = NULL;
    char hostname[HOSTNAME_LEN] = { '\0' };
    if ((fp = fopen("/etc/hostname", "r")) == NULL) {
        strcpy(buf, exception(0, NULL, "hostname"));
    }
    fscanf(fp, "%s", hostname);
    fclose(fp);
    strcpy(buf,hostname);
}

void get_IPv4(char* buf){
    struct ifaddrs *ifaddr, *ifa;
    char ip[100];

    if (getifaddrs(&ifaddr) == -1) {
        strcpy(buf, exception(0, NULL, "interface"));
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){
        if (ifa->ifa_addr == NULL){
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4 인 경우 출력
            if (strncmp(ifa->ifa_name, "eno", 3) == 0 || strncmp(ifa->ifa_name, "Bond", 4) == 0 || strncmp(ifa->ifa_name, "eth", 3) == 0){
                struct sockaddr_in *addr = (struct sockaddr_in *) ifa -> ifa_addr;
                inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
                sprintf(buf, "%s (%s)", ip, ifa->ifa_name);
                return;
            }
        }
    }
    strcpy(buf,exception(0, NULL, "IPv4"));
}

void get_SystemTime(char* buf){
    time_t nowTime;
    struct tm *lt; // Local Time
    char res[TIME_LEN] = { '\0' };
    time(&nowTime);
    lt = localtime(&nowTime);
    if (lt == NULL){
        strcpy(buf, exception(0, NULL, "SystemTime"));
    }
    sprintf(res, "%04d-%02d-%02d %02d:%02d:%02d", lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
    strcpy(buf, res);
}

void get_UpTime(char* buf){
    struct sysinfo info;
    char res[TIME_LEN] = { '\0' };
    if (sysinfo(&info) == -1) {
        strcpy(buf, exception(0, NULL, "sysinfo"));
    }
    sprintf(res, "%d days, %02d:%02d:%02d", (int)(info.uptime / 3600) / 24, (int)(info.uptime % (24 * 3600)) / 3600, (int)(info.uptime % 3600) / 60, (int)info.uptime % 60);
    strcpy(buf,res);
}