#ifndef CPU_INFO_STRUCT_H

#include "common.h"

#define CPU_INFO_STRUCT_H
#define CPU_INFO_LOG "/var/log/00_Server_Management/CPU_Info"

typedef struct CPU_Result {
    DateInfo date;
    float temp;
    float usage;
} CPU_Result;

typedef struct CPU_Usage {
    unsigned long totalJiff;
    unsigned long idleJiff;
} CPU_Usage;

typedef struct CPU_Info {
    CPU_Usage usage;
    DateInfo date;
    float temp;
} CPU_Info;

#endif