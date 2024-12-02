#ifndef RESOURCES_DISPLAY_H

#include "common.h"
#include "network_info_struct.h"
#include <sys/ioctl.h>

#define RESOURCES_DISPLAY_H

#define LEN 4135
#define FUNC_BUF_LEN 128
#define BAR_RATIO 0.5

void display_main(void);
void display_System_Info(struct winsize*, int*, int*);
void display_CPU_Info(struct winsize*, short*, int, int, int);
void display_MEM_Info(struct winsize*, short*, int, int, UNIT, UNIT);
NET_Result* display_NET_Info(struct winsize*, NET_Result *, short*, int, int);

#endif