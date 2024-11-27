#ifndef COMMON_H

#define COMMON_H
#define ERROR_MSG_LEN 128
#define LOG_PATH "/var/log/00_Resources_Status"

typedef enum UNIT { KB, MB, GB, TB } UNIT;

typedef struct DateInfo {
    int year;
    int month;
    int day;
    int hrs;
    int min;
    int sec;
} DateInfo;

typedef struct Unit_Mapping {
    UNIT unit;
    char* str;
} Unit_Mapping;

extern const Unit_Mapping unitMap[];

DateInfo get_Date();
char* exception(int, char *, char *);
double convert_Size_Unit(long long, UNIT);

#endif