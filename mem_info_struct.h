#ifndef MEM_INFO_STRUCT_H

#include "common.h"

#define MEM_INFO_STRUCT_H
#define MEM_INFO_LOG "/var/log/00_Server_Management/MEM_Info"

typedef struct MEM_Size{
    unsigned long mem_total;
    unsigned long mem_free;
    unsigned long swap_total;
    unsigned long swap_free;
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