#include "logrotate.h"

int main() {
    LogConfig config;
    char log_dir[256];

    printf("Enter the full path of the log file: ");
    scanf("%s", config.log_path);

    printf("Enter the maximum size for log rotation (MB): ");
    scanf("%d", &config.max_size_mb);

    printf("Enter the directory containing logs for deletion: ");
    scanf("%s", log_dir);

    printf("Enter the number of days to keep old logs: ");
    scanf("%d", &config.keep_days);

    rotate_log(&config);
    delete_old_logs(log_dir, config.keep_days);

    return 0;
}
