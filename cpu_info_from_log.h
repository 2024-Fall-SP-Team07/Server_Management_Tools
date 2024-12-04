#ifndef CPU_INFO_FROM_LOG_H

#include "cpu_info_struct.h"

#define CPU_INFO_FROM_LOG_H

CPU_Result get_CPU_Information(int, int);
float calc_CPU_Usage(CPU_Usage*, CPU_Usage*);

#endif