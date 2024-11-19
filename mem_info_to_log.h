#ifndef MEM_INFO_TO_LOG_H

#include "common.h"
#include "mem_info_struct.h"

#define MEM_INFO_TO_LOG_H
#define LINE_LEN 64
#define MEM_INFO_LOG "/var/log/00_Resources_Status/MEM_Info"

void write_Mem_Information();
MEM_Size get_Mem_Usage();
void allocation_Value_to_Var(MEM_Size*, char*, long long);

#endif