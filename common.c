#include "common.h"
#include <time.h>
#include <stdio.h>

DateInfo get_Date(){
    time_t current_time;
    DateInfo dateInfo;
    struct tm *tm;
    current_time = time(NULL);
    tm = localtime(&current_time);
    
    dateInfo.year = (tm->tm_year) + 1900;
    dateInfo.month = (tm->tm_mon) + 1;
    dateInfo.day = tm->tm_mday;
    dateInfo.hrs = tm->tm_hour;
    dateInfo.min = tm->tm_min;
    dateInfo.sec = tm->tm_sec;

    return dateInfo;
}

char* exception(int code, char *func_name, char *detail, DateInfo* date){
    static char error_msg[ERROR_MSG_LEN];
    (code == -1) ? sprintf(error_msg, "%04d-%02d-%02d %02d:%02d:%02d (func. - %s) Cannot open file: %s", date->year, date->month, date->day, date->hrs, date->min, date->sec, func_name, detail) : 0;
    (code == -2) ? sprintf(error_msg, "%04d-%02d-%02d %02d:%02d:%02d (func. - %s) Cannot Read data: %s", date->year, date->month, date->day, date->hrs, date->min, date->sec, func_name, detail) : 0;
    (code == -3) ? sprintf(error_msg, "%04d-%02d-%02d %02d:%02d:%02d (func. - %s) Cannot Write data: %s", date->year, date->month, date->day, date->hrs, date->min, date->sec, func_name, detail) : 0;
    (code == -4) ? sprintf(error_msg, "%04d-%02d-%02d %02d:%02d:%02d (func. - %s) Cannot Load data: %s", date->year, date->month, date->day, date->hrs, date->min, date->sec, func_name, detail) : 0;
    (code == -5) ? sprintf(error_msg, "%04d-%02d-%02d %02d:%02d:%02d (func. - %s) Cannot malloc", date->year, date->month, date->day, date->hrs, date->min, date->sec, func_name) : 0;
    return error_msg;
}

double convert_Size_Unit(long long size, UNIT unit){
    double res = (double)size;
    for (int i = 0; i < (int)unit; i++){
        res /= 1024;
    }
    return res;
}