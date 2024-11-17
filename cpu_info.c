/*
- 2024-Fall, System Programming (ELEC0462-003)
- Professor: Jeong, Chang-Su
- Author: Seo, Hyeong-Cheol
- cpu_info.c: Related to CPU information. (Average Usage & Temperature while N minute)
- For detail explanation, Let see the header file. (cpu_info.h)
*/
#include "common.h"
#include "cpu_info.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

CPU_Usage get_CPU_Jiffies(){
    long long user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
    CPU_Usage usage;
    FILE *fp = NULL;
    ((fp = fopen("/proc/stat", "r")) == NULL) ? exception(-1, "/proc/stat") : 0; // /proc/stat File Open Error
    fscanf(fp, "cpu  %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld", &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guest_nice);
    fclose(fp);
    usage.totalJiff = user + nice + system + idle + iowait + irq + softirq + steal + guest + guest_nice;
    usage.idleJiff = idle;
    return usage;
}

void write_CPU_Information(){
    int fd;
    CPU_Info content;
    DIR *dir_ptr = NULL;
    ((dir_ptr = opendir(LOG_PATH)) == NULL) ? mkdir(LOG_PATH, 750) : closedir(dir_ptr); // Create directory if not exists
    ((fd = open(CPU_INFO_LOG, O_WRONLY | O_CREAT | O_APPEND, 640)) == -1) ? exception(-1, "CPU Information Log") : 0; // Create or Open log file
    content.date = get_Date();
    content.temp = get_CPU_Temperature();
    content.usage = get_CPU_Jiffies();
    (write(fd, &content, sizeof(CPU_Info)) != sizeof(CPU_Info)) ? exception(-3, "CPU Information Log") : 0; // Save CPU Information (Temp, Jiff.)
    close(fd);
}

float get_CPU_Temperature(){
    int fd;
    char temp[MAX_TEMP_LEN + 1] = { '\0' }; // for \0
    char path_type[PATH_LEN + 1] = "/sys/class/thermal/thermal_zone0/type"; // for \0
    char path_temp[PATH_LEN + 1] = { '\0' };
    char type[TYPE_LEN + 2] = { '\0' }; // for '\0' and '.'

    for (int i = 1; (fd = open(path_type, O_RDONLY)) != -1; i++){ // Find Temperature File Location
        (read(fd, &type, TYPE_LEN) <= 0) ? exception(-2, "Device Type") : 0;
        type[strlen(type)-1] = '\0'; // remove '\n'
        if ((strcmp(type, "k10temp") == 0) || (strcmp(type, "x86_pkg_temp") == 0) || (strcmp(type, "coretemp") == 0)) {
                break;
        }
        sprintf(path_type, "/sys/class/thermal/thermal_zone%d/type", i);
    }

    (fd == -1) ? exception(-1, "Device Type") : 0;  // Device Type File Open Error

    strncpy(path_temp, path_type, strlen(path_type) - 4); // get CPU Temperature File Location
    strcat(path_temp, "temp");
    close(fd);
    
    ((fd = open(path_temp, O_RDONLY)) == -1) ? exception(-1, "CPU Temperature") : 0; // CPU Temperature File Open Error
    ((fd = read(fd, &temp, MAX_TEMP_LEN)) <= 0) ? exception(-2, "CPU Temperature") : 0; // CPU Temperature Data Read Error
    close(fd);

    return atoi(temp)/1000.0;
}

char* exception(int code, char *detail){
    static char error_msg[ERROR_MAG_LEN];
    (code == -1) ? sprintf(error_msg, "Cannot open file: %s\n", detail) : 0;
    (code == -2) ? sprintf(error_msg, "Cannot Read data: %s\n", detail) : 0;
    (code == -3) ? sprintf(error_msg, "Cannot Write data: %s\n", detail) : 0;
    printf("%s\n", error_msg);
    return error_msg;
}