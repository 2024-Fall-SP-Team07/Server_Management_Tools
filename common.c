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

char* exception(int code, char *func_name, char *detail){
    static char error_msg[ERROR_MSG_LEN];
    (code == -1) ? sprintf(error_msg, "(func. - %s) Cannot open file: %s\n", func_name, detail) : 0;
    (code == -2) ? sprintf(error_msg, "(func. - %s) Cannot Read data: %s\n", func_name, detail) : 0;
    (code == -3) ? sprintf(error_msg, "(func. - %s) Cannot Write data: %s\n", func_name, detail) : 0;
    printf("%s\n", error_msg);
    return error_msg;
}