#ifndef CPU_INFO_STRUCT_H

#include "common.h"

#define CPU_INFO_STRUCT_H

typedef struct CPU_Result {
     float temp;
     float usage;
} CPU_Result;

typedef struct CPU_Usage {
    long long totalJiff;
    long long idleJiff;
} CPU_Usage;

typedef struct CPU_Info {
    CPU_Usage usage;
    DateInfo date;
    float temp;
} CPU_Info;

#endif