#ifndef COMMON_H

#define COMMON_H
#define ERROR_MSG_LEN 256
#define LOG_PATH "/var/log/00_Server_Management"
#define UNIT_COUNT 6

typedef enum UNIT { KB, MB, GB, TB, PB, EB } UNIT;

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
char* exception(int, char *, char *, DateInfo* date);
double convert_Size_Unit(long long, UNIT);

#endif