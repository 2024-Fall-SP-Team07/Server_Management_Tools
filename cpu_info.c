/*
- 2024-Fall, System Programming (ELEC0462-003)
- Professor: Jeong, Chang-Su
- Author: Seo, Hyeong-Cheol
- cpu_info.c: Related to CPU information. (Average Usage & Temperature while N minute)
- For detail explanation, Let see the header file. (cpu_info.h)
*/
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MAX_TEMP_LEN 6
#define PATH_LEN 37
#define TYPE_LEN 13

float get_CPU_Temperature(){
    int fd;
    char temp[MAX_TEMP_LEN + 1]; // for \0
    char path_type[PATH_LEN + 1] = "/sys/class/thermal/thermal_zone0/type"; // for \0
    char path_temp[PATH_LEN + 1];
    char type[TYPE_LEN + 2]; // for '\0' and '.'

    for (int i = 1; (fd = open(path_type, O_RDONLY)) != -1; i++){ // find temperature file number
        if (read(fd, &type, TYPE_LEN) <= 0) {
            return -2;
        }
        type[strlen(type)-1] = '\0'; // remove '\n'
        if ((strcmp(type, "k10temp") == 0) || (strcmp(type, "x86_pkg_temp") == 0) || (strcmp(type, "coretemp") == 0)) {
                break;
        }
        sprintf(path_type, "/sys/class/thermal/thermal_zone%d/type", i);
    }

    if (fd == -1) { // type file open error.
        return -1;
    }

    strncpy(path_temp, path_type, strlen(path_type) - 4); // get CPU Temperature File
    strcat(path_temp, "temp");

    if ((fd = open(path_temp, O_RDONLY)) == -1) { // get CPU Temperature (Unit: milli Celsius)
        return -1;
    }

    if (read(fd, &temp, MAX_TEMP_LEN) <= 0) { // CPU Temp. Reading ERROR
        return -2;
    }

    return atoi(temp)/1000.0;
}

float get_CPU_Usage(){
    int fd;
    float usage;
    fd = open("/proc/stat", O_RDONLY);
}