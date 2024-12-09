#ifndef RESOURCES_DISPLAY_H

#include "common.h"
#include "network_info.h"
#include <termio.h>

#define RESOURCES_DISPLAY_H

#define LEN 4135
#define FUNC_BUF_LEN 128
#define BAR_RATIO 0.5
#define DISK_BAR_RATIO 0.25
#define CPU_TEMP_CRITICAL_POINT 70
#define CPU_USAGE_CRITICAL_PERCENT 85
#define MEM_USAGE_CRITICAL_PERCENT 85
#define SWAP_USAGE_CRITICAL_PERCENT 0
#define DISK_USAGE_CRITICAL_PERCENT 85
#define CPU_MEM_LINE 17
#define PARTITION_LINE 7

void signal_handling(int);
void restore_screen_init(void);
void initialization(void);
void display_main(void);
void display_clear(struct winsize*, int);
void display_System_Info(int, int);
short check_running_collector(void*, char*);
void display_CPU_Info(struct winsize*, int*, int, int, int, int);
void display_MEM_Info(struct winsize*, int, int, UNIT, UNIT);
NET_Result* display_NET_Info(struct winsize*, NET_Result*, short*, int, int);
void display_Disk_Info(void);
void change_settings(void);
int find_idx_unit(char*);

#endif