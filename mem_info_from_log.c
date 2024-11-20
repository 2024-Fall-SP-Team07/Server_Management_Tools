#include "mem_info_from_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

MEM_Size get_Mem_Information(int boundary){
    int fd = -1, i = 0;
    MEM_Size res;
    MEM_Info buf;
    long long *physical_usage_list = (long long*)malloc(boundary * sizeof(float));
    long long *swap_usage_list = (long long*)malloc(boundary * sizeof(float));
    ((fd = open(MEM_INFO_LOG, O_RDONLY)) == -1) ? exception(-1, "get_Mem_Information", "Memory Information Log") : 0;
    lseek(fd, -(sizeof(MEM_Info) * boundary), SEEK_END);
    for (i = 0; i < boundary; i++){
        (read(fd, &buf, sizeof(MEM_Info)) != sizeof(MEM_Info)) ? exception(-2, "get_Mem_Information", "Memory Information Log") : 0;
        physical_usage_list[i] = buf.size.mem_free;
        swap_usage_list[i] = buf.size.swap_free;
    }
    close(fd);
    res.mem_free = calc_Mem_Avg_Value(physical_usage_list, i);
    res.swap_free = calc_Mem_Avg_Value(swap_usage_list, i);
    res.mem_total = buf.size.mem_total;
    res.swap_total = buf.size.swap_total;
    free(physical_usage_list);
    free(swap_usage_list);
    printf("%d-%d-%d %d:%d:%d ", buf.date.year, buf.date.month, buf.date.day, buf.date.hrs, buf.date.min, buf.date.sec);
    return res;
}

long long calc_Mem_Avg_Value(long long *list, int idx){
    long long sum = 0;
    for (int i = 0; i < idx; i++){
        sum += list[i];
    }
    return (long long)(sum / idx);
}