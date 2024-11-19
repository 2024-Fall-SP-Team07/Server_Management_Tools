#ifndef CPU_INFO_TO_LOG_H

#include "cpu_info_struct.h"

#define CPU_INFO_TO_LOG_H
#define MAX_TEMP_LEN 6
#define PATH_LEN 37
#define TYPE_LEN 13
#define LOG_MSG_LEN 28 // YYYY-DD-MM HH:MM:SS 100.00\n(<- Temperature, Celcius)
#define CPU_INFO_LOG "/var/log/00_Resources_Status/CPU_Info"

CPU_Usage get_CPU_Jiffies();
void write_CPU_Information();
float get_CPU_Temperature();

#endif