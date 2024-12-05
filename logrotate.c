#include "logrotate.h"
#include <sys/stat.h>
#include <unistd.h>

void generate_log_filename(const char *base_name, int index, char *out_filename) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);

    char date_str[16];
    strftime(date_str, sizeof(date_str), "%Y%m%d", tm_info);

    if (index >= 0) {
        snprintf(out_filename, 256, "%s_%s_%d.log", base_name, date_str, index);
    } else {
        snprintf(out_filename, 256, "%s_%s.log", base_name, date_str);
    }
}

void delete_old_logs(const char *log_dir, int keep_days) {
    char command[512];
    snprintf(command, sizeof(command), "find %s -name '*.log' -mtime +%d -exec rm -f {} \\;", log_dir, keep_days);
    system(command);
    printf("Deleted logs older than %d days in directory: %s\n", keep_days, log_dir);
}

void rotate_log(LogConfig *config) {
    struct stat st;
    if (stat(config->log_path, &st) != 0) {
        perror("Error accessing log file");
        return;
    }

    int size_mb = st.st_size / (1024 * 1024);
    printf("Current file size of '%s': %d MB\n", config->log_path, size_mb);

    if (size_mb > config->max_size_mb) {
        int index = 0;
        char new_filename[256];
        generate_log_filename(config->log_path, index, new_filename);

        while (access(new_filename, F_OK) == 0) {
            index++;
            generate_log_filename(config->log_path, index, new_filename);
        }

        rename(config->log_path, new_filename);
        fclose(fopen(config->log_path, "w"));
        printf("Rotated log file: %s -> %s\n", config->log_path, new_filename);
    } else {
        printf("Log file does not exceed the maximum size (%d MB). No rotation performed.\n", config->max_size_mb);
    }
}
