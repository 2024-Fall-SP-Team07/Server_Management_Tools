#ifndef COMMON_H

#define COMMON_H
#define ERROR_MSG_LEN 128

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