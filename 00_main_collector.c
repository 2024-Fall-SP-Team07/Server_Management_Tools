/*
Author: Seo, Hyeong-Cheol
*/
#include "cpu_info_to_log.h"
#include "mem_info_to_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>

#define ERROR_LOG_PATH_COLLECTOR "/var/log/00_Server_Management/zz_resources_collector_error.log"

int main(void){
    int fd = -1;
    DIR* dir_ptr = NULL;
    DateInfo date = get_Date();
    pthread_t t1;
    pthread_t t2;

    if(geteuid() != 0) {
        printf("\nThis program must be running with root privileges. (using sudo or as root)...\nexit.\n\n");
        exit(-1);
    }

    close(1);
    ((dir_ptr = opendir(LOG_PATH)) == NULL) ? mkdir(LOG_PATH, (S_IRWXU | S_IRGRP | S_IXGRP) & (~S_IRWXO) ) : closedir(dir_ptr);
    if ((fd = open(ERROR_LOG_PATH_COLLECTOR, O_WRONLY | O_CREAT | O_APPEND, (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP ) & (~S_IRWXO))) == -1) {
        fd = open("/dev/tty", O_WRONLY);
        if (dup2(fd, 1) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        printf("%04d-%02d-%02d %02d:%02d:%02d Fail to open/create: %s\n", date.year, date.month, date.day, date.hrs, date.min, date.sec, ERROR_LOG_PATH_COLLECTOR);
        return 0;
    }
    while(1){
        pthread_create(&t1, NULL, write_CPU_Information, NULL);
        pthread_create(&t2, NULL, write_Mem_Information, NULL);
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        sleep(1);
    }
    return 0;
}