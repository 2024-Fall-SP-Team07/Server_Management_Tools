#ifndef MEM_INFO_TO_LOG_H

#include "common.h"
#include "mem_info_struct.h"

#define MEM_INFO_TO_LOG_H
#define LINE_LEN 64


void *write_Mem_Information();
MEM_Size get_Mem_Usage();
void allocation_Value_to_Var(MEM_Size*, char*, unsigned long);

#endif