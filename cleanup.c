#include "h_for_tmp_cleanup.h"
#include "check_before_cleanup.h"
#include "cleanup.h"

void log_deletion_record(const char *filename)
{
    int log_fd = open("/var/log/00_Server_Management/deleted_tmp_files.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (log_fd == -1)
    {
        // perror("Failed to open log file");
        return;
    }
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "[%04d-%02d-%02d %02d:%02d:%02d] Deleted: %s\n",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, filename);
    if (write(log_fd, log_msg, strlen(log_msg)) == -1)
    {
        // perror("Failed to write to log file");
    }
    close(log_fd);
}

int cleanup_files_recursive(const char *dirpath, int max_age_days, int deleted_count)
{
    DIR *dir = opendir(dirpath);
    if (!dir)
    {
        // perror("opendir failed");
        return deleted_count;
    }
    struct dirent *entry;
    char filepath[512];
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name);
        if (should_exclude_directory(filepath))
        {
            continue;
        }
        struct stat st;
        if (stat(filepath, &st) == -1)
        {
            // perror("stat failed");
            continue;
        }
        if (is_file_in_use(filepath) || is_file_locked(filepath))
        {
            continue;
        }
        else if (S_ISDIR(st.st_mode))
        {
            deleted_count = cleanup_files_recursive(filepath, max_age_days, deleted_count);
            if (is_directory_empty(filepath))
            {
                int is_old_enough = file_age_check(filepath, max_age_days);
                int is_invalid_owner_group = !is_valid_owner_group(filepath);
                if (is_old_enough || is_invalid_owner_group)
                {
                    if (rmdir(filepath) == -1)
                    {
                        // perror("rmdir failed");
                    }
                    else
                    {
                        log_deletion_record(filepath);
                        deleted_count++; // 파일 삭제 시 카운트 증가
                        refresh();
                    }
                }
            }
        }
        else
        {
            int is_old_enough = file_age_check(filepath, max_age_days);
            int is_invalid_owner_group = !is_valid_owner_group(filepath);
            if (is_old_enough || is_invalid_owner_group)
            {
                if (remove(filepath) == -1)
                {
                    // perror("remove failed");
                }
                else
                {
                    log_deletion_record(filepath);
                    deleted_count++; // 파일 삭제 시 카운트 증가
                    refresh();
                }
            }
        }
    }
    closedir(dir);
    return deleted_count;
}

int cleanup_log_files(const char *log_dir, int max_age_days, int deleted_count)
{
    DIR *dir = opendir(log_dir);
    if (!dir)
    {
        // perror("opendir failed");
        return deleted_count;
    }
    struct dirent *entry;
    char filepath[512];
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char *date_part = strrchr(entry->d_name, '-');  // 파일 이름에서 마지막 '-'을 찾음
        if (date_part != NULL)
        {
        date_part++;  // '-' 뒤의 날짜 부분으로 이동 (날짜는 '-' 뒤에 있음)
        if (strlen(date_part) == 8)
        {  // 날짜가 8자리여야 함 (YYYYMMDD)
            int year, month, day;
            // 날짜를 YYYYMMDD 형식으로 파싱
            if (sscanf(date_part, "%4d%2d%2d", &year, &month, &day) == 3)
            {
                    if (!is_valid_date(year, month, day))
                    {
                        snprintf(filepath, sizeof(filepath), "%s/%s", log_dir, entry->d_name);
                        if (is_file_in_use(filepath) || is_file_locked(filepath))
                        {
                            continue;
                        }
                        if (remove(filepath) == -1)
                        {
                            // perror("remove failed");
                        }
                        else
                        {
                            log_deletion_record(filepath);
                            deleted_count++; // 파일 삭제 시 카운트 증가
                            refresh();
                        }
                    }
                    else
                    {
                        snprintf(filepath, sizeof(filepath), "%s/%s", log_dir, entry->d_name);
                        if (is_file_in_use(filepath) || is_file_locked(filepath))
                        {
                            continue;
                        }
                        else
                        {
                            time_t current_time;
                            struct tm current_tm;

                            time(&current_time);
                            localtime_r(&current_time, &current_tm);

                            struct tm file_tm = { 0 };
                            file_tm.tm_year = year - 1900;  // tm_year는 1900년을 기준으로 계산됨
                            file_tm.tm_mon = month - 1;    // tm_mon은 0부터 시작
                            file_tm.tm_mday = day;
                            
                            time_t file_time = mktime(&file_tm);

                            double diff_days = difftime(current_time, file_time) / (60 * 60 * 24);

                            if (diff_days > max_age_days)
                            {
                                if (remove(filepath) == -1)
                                {
                                // perror("remove failed");
                                }
                                else
                                {
                                    log_deletion_record(filepath);
                                    deleted_count++; // 파일 삭제 시 카운트 증가
                                    refresh();
                                }
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