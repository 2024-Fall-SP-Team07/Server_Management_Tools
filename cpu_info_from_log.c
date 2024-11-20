#include "cpu_info_from_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

CPU_Result get_CPU_Information(int boundary){
    int fd = -1, i = 0;
    CPU_Info priv_buf, cur_buf;
    CPU_Result res;
    float *usage_list = (float*)malloc((boundary + 1) * sizeof(float));
    float *temp_list = (float*)malloc((boundary + 1) * sizeof(float));
    ((fd = open(CPU_INFO_LOG, O_RDONLY)) == -1) ? exception(-1, "get_CPU_Information", "CPU Information Log") : 0;
    lseek(fd, -(sizeof(CPU_Info) * (boundary + 1)), SEEK_END);
    for (i = 0; i < boundary; i++){
        (read(fd, &priv_buf, sizeof(CPU_Info)) != sizeof(CPU_Info)) ? exception(-2, "get_CPU_Information (Priv.)", "CPU Information Log") : 0;
        (read(fd, &cur_buf, sizeof(CPU_Info)) != sizeof(CPU_Info)) ? exception(-2, "get_CPU_Information (Cur.)", "CPU Information Log") : 0;
        lseek(fd, -sizeof(CPU_Info), SEEK_CUR);
        usage_list[i] = calc_CPU_Usage(&(priv_buf.usage), &(cur_buf.usage));
        temp_list[i] = cur_buf.temp;
    }
    close(fd);
    res.temp = calc_CPU_Avg_Value(temp_list, i);
    res.usage = calc_CPU_Avg_Value(usage_list, i);
    free(usage_list);
    free(temp_list);
    printf("%d-%d-%d %d:%d:%d ", cur_buf.date.year, cur_buf.date.month, cur_buf.date.day, cur_buf.date.hrs, cur_buf.date.min, cur_buf.date.sec);
    return res;
}

float calc_CPU_Avg_Value(float *list, int idx){
    float sum = 0;
    for (int i = 0; i < idx; i++){
        sum += list[i];
    }
    return sum / (float)idx;
}

float calc_CPU_Usage(CPU_Usage* priv_log, CPU_Usage* cur_log){
    float usage = 0;
    usage = 100 * (1 - (((cur_log->idleJiff) - (priv_log->idleJiff)) / (float)((cur_log->totalJiff) - (priv_log->totalJiff))));
    return usage;
}