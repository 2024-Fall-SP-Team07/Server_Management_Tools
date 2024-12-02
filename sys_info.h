#ifndef SYS_INFO_H

#define SYS_INFO_H
#define HOSTNAME_LEN 128
#define TIME_LEN 20

void get_Hostname(char*, int);
void get_ProductName(char*, int);
void get_SystemTime(char*);
void get_UpTime(char*);

#endif