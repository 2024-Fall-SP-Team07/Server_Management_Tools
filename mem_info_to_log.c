#include "mem_info_to_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

void write_Mem_Information() {
    int fd = -1;
    MEM_Info content;
    DIR *dir_ptr = NULL;
    ((dir_ptr = opendir(LOG_PATH)) == NULL) ? mkdir(LOG_PATH, (S_IRWXU | S_IRGRP | S_IXGRP) & (~S_IRWXO) ) : closedir(dir_ptr); // Create directory if not exists (750)
    ((fd = open(MEM_INFO_LOG, O_WRONLY | O_CREAT | O_APPEND, (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP ) & (~S_IRWXO))) == -1) ? printf("%s\n", exception(-1, "write_Mem_Information", "Memory Information Log")) : 0; 
    // Create or Open log file (640)
    content.date = get_Date();
    content.size = get_Mem_Usage();
    (write(fd, &content, sizeof(MEM_Info)) != sizeof(MEM_Info)) ? printf("%s\n", exception(-3, "write_Mem_Information", "Memory Information Log")) : 0; // Save Memory Information (Free, Total Size)
    close(fd);
}

MEM_Size get_Mem_Usage() {
    FILE* fp = NULL;
    char buf[LINE_LEN] = { '\0' };
    long long size_buf;
    MEM_Size res = { -1, -1, -1, -1 };
    ((fp = fopen("/proc/meminfo", "r")) == NULL) ? printf("%s\n", exception(-1, "get_Mem_Usage", "Memory Usage")) : 0;
    for (; fscanf(fp, "%s %ld", buf, &size_buf) != EOF; allocation_Value_to_Var(&res, buf, size_buf));
    fclose(fp);    
    ((res.mem_free == -1) || (res.mem_total == -1) || (res.swap_free == -1) || (res.swap_total == -1)) ? printf("%s\n", exception(-2, "get_Mem_Usage", "Memory Size(Usage)")) : 0;
    return res;
}

void allocation_Value_to_Var(MEM_Size* res, char* buf, long long size_buf){
    if (strcmp(buf, "MemTotal:") == 0) {
        res->mem_total = size_buf;
        return;
    }
    if (strcmp(buf, "MemAvailable:") == 0) {
        res->mem_free = size_buf;
        return;
    }
    if (strcmp(buf, "SwapTotal:") == 0) {
        res->swap_total = size_buf;
        return;
    }
    if (strcmp(buf, "SwapFree:") == 0) {
        res->swap_free = size_buf;
        return;
    }
}