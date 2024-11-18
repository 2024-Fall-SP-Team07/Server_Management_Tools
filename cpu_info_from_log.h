#ifndef CPU_INFO_FROM_LOG_H

#include "cpu_info_struct.h"

#define CPU_INFO_FROM_LOG_H
#define LOG_PATH "/var/log/00_Resources_Status"
#define CPU_INFO_LOG "/var/log/00_Resources_Status/CPU_Info"

CPU_Result get_CPU_Information(int);
float calc_CPU_Avg_Usage(float*, int);
float calc_CPU_Avg_Temp(float*, int);
float calc_CPU_Usage(CPU_Usage*, CPU_Usage*);

#endif