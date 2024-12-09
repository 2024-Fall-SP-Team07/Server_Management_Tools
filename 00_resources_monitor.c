#include "resources_display.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#define ERROR_LOG_PATH_MONITOR "/var/log/00_Server_Management/zz_resources_monitor_error.log"

int main(void){
    int fd = -1;
    DIR* dir_ptr = NULL;
    DateInfo date = get_Date();
    
    if(geteuid() != 0) {
        printf("\nThis program must be running with root privileges. (using sudo or as root)...\nexit.\n\n");
        exit(-1);
    }

    close(2);
    ((dir_ptr = opendir(LOG_PATH)) == NULL) ? mkdir(LOG_PATH, (S_IRWXU | S_IRGRP | S_IXGRP) & (~S_IRWXO) ) : closedir(dir_ptr);
    if ((fd = open(ERROR_LOG_PATH_MONITOR, O_WRONLY | O_CREAT | O_APPEND, (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP ) & (~S_IRWXO))) == -1) {
        fd = open("/dev/tty", O_WRONLY);
        if (dup2(fd, 2) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "%04d-%02d-%02d %02d:%02d:%02d Fail to open/create: %s\n", date.year, date.month, date.day, date.hrs, date.min, date.sec, ERROR_LOG_PATH_MONITOR);
        return 0;
    }
    initialization();
    display_main();
    return 0;
}