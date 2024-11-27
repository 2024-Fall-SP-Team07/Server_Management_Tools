#include "disk_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>

// Signal -> 디스크 용량 못 불러오면 signal 호출, 바로 null 반환

DISK_Result* get_Partition_Info_List(){
    FILE *fp = NULL;
    DISK_Result *res, *head = NULL, *cur = NULL;
    char fileSystem[DISK_NAME_LEN] = { '\0' }, path_buf[DISK_PATH_LEN] = { '\0' };
    if ((fp = fopen("/proc/mounts", "r")) == NULL) {
        // strcpy(buf, exception(-2, NULL, "mounts"));
        return NULL;
    }
    while (fscanf(fp, "%s %s %*s %*s %*s %*s", fileSystem, path_buf) != EOF) {
        if ((res = (DISK_Result*)malloc(sizeof(DISK_Result))) == NULL){
            return NULL;
        }   
        if (strncmp("/dev", fileSystem, 4) != 0 && strncmp("tmpfs", fileSystem, 5) != 0) {
            continue;
        }
        strcpy(res->fileSystem, fileSystem);
        strcpy(res->mount_path, path_buf);
        res->size = get_Partition_Size(path_buf);

        if (((int)(res->size.free_size) == -1) || (int)((res->size.total_space) == -1)) {
            return NULL;
        }

        res->next = NULL;
        if (head == NULL) {
            head = res;
            cur = res;
        } else {
            cur->next = res;
            cur = cur->next;
        }
    }
    return head;
}

DISK_SPACE get_Partition_Size(char* path){
    DISK_SPACE res;
    struct statvfs buf;
    if (statvfs(path, &buf) == -1){
        res.total_space = -1;
        res.free_size = -1;
    } else {
        res.total_space = (unsigned long long) (buf.f_blocks * buf.f_frsize) / 1024; // Unit: KB
        res.free_size = (unsigned long long) (buf.f_bfree * buf.f_frsize) / 1024; // Unit: KB
    }
    return res;
}

short get_Partition_Max_Length(DISK_Result* head){
    DISK_Result* temp = head;
    short max = 0;
    for (; temp != NULL; temp = temp->next){
        max = (((short)strlen(temp->mount_path) > max) ? (short)strlen(temp->mount_path) : max);
    }
    return max;
}