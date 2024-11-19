#ifndef COMMON_H

#define COMMON_H
#define ERROR_MSG_LEN 128
#define LOG_PATH "/var/log/00_Resources_Status"

typedef struct DateInfo {
    int year;
    int month;
    int day;
    int hrs;
    int min;
    int sec;
} DateInfo;

DateInfo get_Date();
char* exception(int, char *, char *);

#endif