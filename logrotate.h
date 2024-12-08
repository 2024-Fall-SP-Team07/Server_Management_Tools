#ifndef LOGROTATE_H
#define LOGROTATE_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    char log_path[256];
    int max_size_mb;
    int keep_days;
} LogConfig;

void generate_log_filename(const char *base_name, int index, char *out_filename);
void delete_old_logs(const char *log_dir, int keep_days);
void rotate_log(LogConfig *config);

#endif 
