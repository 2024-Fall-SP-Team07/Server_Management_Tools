#include "h_for_tmp_cleanup.h"
#include "cleanup.h"

int cleanup_files_recursive(const char *dirpath, int max_age_days, int deleted_count) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        // perror("opendir failed");
        return deleted_count;
    }
    struct dirent *entry;
    char filepath[512];
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);
        if (should_exclude_directory(filepath)) {
            continue;
        }
        struct stat st;
        if (stat(filepath, &st) == -1) {
            // perror("stat failed");
            continue;
        }
        if (is_file_in_use(filepath) || is_file_locked(filepath)) {
            continue;
        } else if (S_ISDIR(st.st_mode)) {
            deleted_count = cleanup_files_recursive(filepath, max_age_days, deleted_count);
            if (is_directory_empty(filepath)) {
                int is_old_enough = file_age_check(filepath, max_age_days);
                int is_invalid_owner_group = !is_valid_owner_group(filepath);
                if (is_old_enough || is_invalid_owner_group) {
                    if (rmdir(filepath) == -1) {
                        // perror("rmdir failed");
                    } else {
                        log_deletion_record(filepath);
                        deleted_count++; // 파일 삭제 시 카운트 증가
                    }
                }
            }
        } else {
            int is_old_enough = file_age_check(filepath, max_age_days);
            int is_invalid_owner_group = !is_valid_owner_group(filepath);
            if (is_old_enough || is_invalid_owner_group) {
                if (remove(filepath) == -1) {
                    // perror("remove failed");
                } else {
                    log_deletion_record(filepath);
                    deleted_count++; // 파일 삭제 시 카운트 증가
                }
            }
        }
    }
    closedir(dir);
    return deleted_count;
}

int cleanup_log_files(const char *log_dir, int max_age_days, int deleted_count) {
    DIR *dir = opendir(log_dir);
    if (!dir) {
        // perror("opendir failed");
        return deleted_count;
    }
    struct dirent *entry;
    char filepath[512];
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char *last_dash = strrchr(entry->d_name, '-');
        char *dot_log = strrchr(entry->d_name, '.');
        if (last_dash != NULL && dot_log != NULL && last_dash < dot_log) {
            char date_part[11];
            strncpy(date_part, last_dash + 1, 10);
            date_part[10] = '\0';
            if (strlen(date_part) == 10 && date_part[4] == '-' && date_part[7] == '-') {
                int year, month, day;
                if (sscanf(date_part, "%d-%d-%d", &year, &month, &day) == 3) {
                    if (!is_valid_date(year, month, day)) {
                        snprintf(filepath, sizeof(filepath), "%s/%s", log_dir, entry->d_name);
                        if (is_file_in_use(filepath) || is_file_locked(filepath)) {
                            continue;
                        }
                        if (remove(filepath) == -1) {
                            // perror("remove failed");
                        } else {
                            log_deletion_record(filepath);
                            deleted_count++; // 파일 삭제 시 카운트 증가
                        }
                    } else {
                        snprintf(filepath, sizeof(filepath), "%s/%s", log_dir, entry->d_name);
                        if (is_file_in_use(filepath) || is_file_locked(filepath)) {
                            continue;
                        }
                        if (file_age_check(filepath, max_age_days)) {
                            if (remove(filepath) == -1) {
                                // perror("remove failed");
                            } else {
                                log_deletion_record(filepath);
                                deleted_count++; // 파일 삭제 시 카운트 증가
                            }
                        }
                    }
                }
            }
        }
    }
    closedir(dir);
    return deleted_count;
}