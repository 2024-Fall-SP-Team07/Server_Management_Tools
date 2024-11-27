#ifndef DISK_INFO_STRUCT_H

#define DISK_INFO_STRUCT_H
#define DISK_NAME_LEN 32
#define DISK_PATH_LEN 4096

typedef struct DISK_SPACE {
    unsigned long long total_space;
    unsigned long long free_size;
} DISK_SPACE;

typedef struct DISK_Result {
    DISK_SPACE size;
    char mount_path[DISK_PATH_LEN];
    char fileSystem[DISK_NAME_LEN];
    struct DISK_Result *next;
} DISK_Result;

#endif