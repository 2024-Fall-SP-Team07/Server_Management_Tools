#ifndef COMMON_H
#define COMMON_H

typedef struct DateInfo {
    int year;
    int month;
    int day;
    int hrs;
    int min;
    int sec;
} DateInfo;

DateInfo get_Date();

#endif