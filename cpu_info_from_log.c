#include "cpu_info_from_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

CPU_Result get_CPU_Information(int boundary){
    int fd = -1, idx = 0;
    CPU_Info priv_buf, cur_buf;
    CPU_Result res;
    double sum_usage = 0, sum_temp = 0;
    ((fd = open(CPU_INFO_LOG, O_RDONLY)) == -1) ? exception(-1, "get_CPU_Information", "CPU Information Log") : 0;
    lseek(fd, -(sizeof(CPU_Info) * (boundary + 1)), SEEK_END);
    for (idx = 0; idx < boundary; idx++){
        (read(fd, &priv_buf, sizeof(CPU_Info)) != sizeof(CPU_Info)) ? exception(-2, "get_CPU_Information (Priv.)", "CPU Information Log") : 0;
        (read(fd, &cur_buf, sizeof(CPU_Info)) != sizeof(CPU_Info)) ? exception(-2, "get_CPU_Information (Cur.)", "CPU Information Log") : 0;
        lseek(fd, -sizeof(CPU_Info), SEEK_CUR);
        sum_usage += calc_CPU_Usage(&(priv_buf.usage), &(cur_buf.usage));
        sum_temp += cur_buf.temp;
    }
    close(fd);
    res.temp = sum_temp / idx;
    res.usage = sum_usage / idx;
    res.date = cur_buf.date;
    return res;
}

float calc_CPU_Usage(CPU_Usage* priv_log, CPU_Usage* cur_log){
    float usage = 0;
    usage = 100 * (1 - (((cur_log->idleJiff) - (priv_log->idleJiff)) / (float)((cur_log->totalJiff) - (priv_log->totalJiff))));
    return usage;
}