#include "mem_info_from_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

MEM_Info get_Mem_Information(int boundary){
    int fd = -1, idx = 0;
    MEM_Info buf, res;
    long long sum_physical_usage = 0, sum_swap_usage = 0;
    ((fd = open(MEM_INFO_LOG, O_RDONLY)) == -1) ? exception(-1, "get_Mem_Information", "Memory Information Log") : 0;
    lseek(fd, -(sizeof(MEM_Info) * boundary), SEEK_END);
    for (idx = 0; idx < boundary; idx++){
        (read(fd, &buf, sizeof(MEM_Info)) != sizeof(MEM_Info)) ? exception(-2, "get_Mem_Information", "Memory Information Log") : 0;
        sum_physical_usage = buf.size.mem_free;
        sum_swap_usage = buf.size.swap_free;
    }
    close(fd);
    res.size.mem_free = sum_physical_usage / idx;
    res.size.swap_free = sum_swap_usage / idx;
    res.size.mem_total = buf.size.mem_total;
    res.size.swap_total = buf.size.swap_total;
    return res;
}