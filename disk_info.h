#ifndef DISK_INFO_H

#include "disk_info_struct.h"

#define DISK_INFO_H

DISK_Result* get_Partition_Info_List();
DISK_SPACE get_Partition_Size(char*);
short get_Partition_Max_Length(DISK_Result*);

#endif