#ifndef CPU_INFO_H
#include "common.h"

#define CPU_INFO_H
#define MAX_TEMP_LEN 6
#define PATH_LEN 37
#define TYPE_LEN 13
#define ERROR_MAG_LEN 40
#define LOG_MSG_LEN 28 // YYYY-DD-MM HH:MM:SS 100.00\n(<- Temperature, Celcius)
#define LOG_PATH "/var/log/00_Resources_Status"
#define CPU_INFO_LOG "/var/log/00_Resources_Status/CPU_Info"

typedef struct CPU_Usage {
    long long totalJiff;
    long long idleJiff;
} CPU_Usage;

typedef struct CPU_Info {
    CPU_Usage usage;
    DateInfo date;
    float temp;
} CPU_Info;

CPU_Usage get_CPU_Jiffies();
void write_CPU_Information();
float get_CPU_Temperature();
char* exception(int code, char *detail);

#endif