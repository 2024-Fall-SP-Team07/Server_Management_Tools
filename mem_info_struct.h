#ifndef MEM_INFO_STRUCT_H

#include "common.h"

#define MEM_INFO_STRUCT_H
#define MEM_INFO_LOG "/var/log/00_Resources_Status/MEM_Info"

typedef enum UNIT { KB, MB, GB, TB } UNIT;

typedef struct MEM_Size{
    long long mem_total;
    long long mem_free;
    long long swap_total;
    long long swap_free;
} MEM_Size;

typedef struct MEM_Info{
    DateInfo date;
    MEM_Size size;
} MEM_Info;

typedef struct MEM_Result {
    MEM_Size size;
    DateInfo date;
    UNIT unit;
} MEM_Result;

typedef struct Unit_Mapping {
    UNIT unit;
    char* str;
} Unit_Mapping;

extern const Unit_Mapping unitMap[];

#endif