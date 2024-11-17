#include "common.h"
#include <time.h>
#include <stdlib.h>

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