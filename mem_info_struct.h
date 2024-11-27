#ifndef MEM_INFO_STRUCT_H

#include "common.h"

#define MEM_INFO_STRUCT_H
#define MEM_INFO_LOG "/var/log/00_Resources_Status/MEM_Info"

typedef struct MEM_Size{
    long long mem_total;
    long long mem_free;
    long long swap_total;
    long long swap_free;
} MEM_Size;

typedef struct MEM_Info{
    MEM_Size size;
    DateInfo date;
} MEM_Info;

typedef struct MEM_Result {
    MEM_Size size;
    DateInfo date;
    UNIT unit;
} MEM_Result;

#endif