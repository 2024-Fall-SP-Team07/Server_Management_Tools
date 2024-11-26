/*
/tmp, /var/tmp, /var/cache에서 조건에 따른 파일 삭제 구현 완료
시간 경과, 프로그램의 실행 및 잠금 여부, uid 및 gid 확인
/var/log에서 조건에 따른 파일 삭제 구현 완료
형식 확인, 시간 경과, 프로그램의 실행 및 잠금 여부
/dev파일은 /dev/null같은 파일의 경우 major, minor number가 0일 수 있고 아예 없는 경우를 찾는법을 찾지 못해 미구현

ncurses를 사용한 임시 파일의 삭제 여부확인, 삭제 진행도 표시, 삭제 완료 표시 구현 완료
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <sys/file.h>
#include <curses.h>

int is_valid_date(int year, int month, int day) {
    if (month < 1 || month > 12) {
        return 0;
    }
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        days_in_month[1] = 29;
    }
    if (day < 1 || day > days_in_month[month - 1]) {
        return 0;
    }
    return 1;
}

void log_deletion_record(const char *filename) {
    int log_fd = open("/var/log/deleted_tmp_files.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (log_fd == -1) {
        // perror("Failed to open log file");
        return;
    }
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "[%04d-%02d-%02d %02d:%02d:%02d] Deleted: %s\n",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, filename);
    if (write(log_fd, log_msg, strlen(log_msg)) == -1) {
        // perror("Failed to write to log file");
    }
    close(log_fd);
}

int file_age_check(const char *filename, int max_age_days) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        // perror("stat failed");
        return -1;
    }
    time_t current_time = time(NULL);
    double diff = difftime(current_time, st.st_mtime) / (60 * 60 * 24);
    return (diff >= max_age_days);
}

int is_file_in_use(const char *filename) {
    char command[512];
    snprintf(command, sizeof(command), "fuser %s 2>/dev/null", filename);
    refresh();
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        // perror("fuser failed");
        return 0;
    }
    char buffer[128];
    int is_in_use = 0;
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        is_in_use = 1;
    }
    pclose(fp);
    return is_in_use;
}

int is_file_locked(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        // perror("open failed");
        return 0;
    }
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_RDLCK;
    if (fcntl(fd, F_GETLK, &lock) == -1) {
        // perror("fcntl failed");
        close(fd);
        return 0;
    }
    close(fd);
    return (lock.l_type != F_UNLCK);
}

int is_valid_owner_group(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == -1) {
        // perror("stat failed");
        return 0;
    }
    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    if (pw == NULL || gr == NULL) {
        return 0;
    }
    return 1;
}

int is_directory_empty(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        // perror("opendir failed");
        return 0;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            closedir(dir);
            return 0;
        }
    }
    closedir(dir);
    return 1;
}

int should_exclude_directory(const char *filepath) {
    const char *exclude_dirs[] = {
        "/var/cache/apt/archives",
        "/var/cache/dnf",
        "/var/cache/ldconfig",
        "/var/cache/pam",
        "/var/cache/private",
        "/var/cache/snapd",
        "/var/cache/PackageKit",
        "/var/cache/fontconfig"
    };
    for (int i = 0; i < sizeof(exclude_dirs) / sizeof(exclude_dirs[0]); i++) {
        if (strncmp(filepath, exclude_dirs[i], strlen(exclude_dirs[i])) == 0) {
            return 1;
        }
    }
    return 0;
}

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

int ask_delete_confirmation(int* line) {
    int ch;
    char message[128];

    clear();
    snprintf(message, sizeof(message), "Do you want to delete the files? (y/n) : ");
    mvprintw(*line, 0, "%s", message);
    refresh();
    
    ch = getch();
    if (ch == 'y' || ch == 'Y')
    {
        clear();
        snprintf(message, sizeof(message), "Do you want to delete the files? (y/n) : %c", ch);
        mvprintw((*line)++, 0, "%s", message);
        refresh();

        return 1;
    }
    else if (ch == 'n' || ch == 'N')
    {
        clear();
        snprintf(message, sizeof(message), "Do you want to delete the files? (y/n) : %c", ch);
        mvprintw((*line)++, 0, "%s", message);
        refresh();

        return 0;
    }

    (*line)++;
    return ask_delete_confirmation(line); // 잘못된 입력 시 다시 묻기
}

int main() {
    initscr();
    cbreak();
    curs_set(0);

    int l = 0;
    int delete_files = ask_delete_confirmation(&l);
    int deleted_files_count = 0; // 삭제된 파일 개수 추적

    char message[128];

    if (delete_files) {
        snprintf(message, sizeof(message), "Deleting tmp files from /tmp...");
        mvprintw(l++, 0, "%s", message);
        refresh();
        deleted_files_count = cleanup_files_recursive("/tmp", 1, deleted_files_count);
        
        snprintf(message, sizeof(message), "Deleting tmp files from /var/tmp...");
        mvprintw(l++, 0, "%s", message);
        refresh();
        deleted_files_count = cleanup_files_recursive("/var/tmp", 7, deleted_files_count);
        
        snprintf(message, sizeof(message), "Deleting tmp files from /var/cache...");
        mvprintw(l++, 0, "%s", message);
        refresh();
        deleted_files_count = cleanup_files_recursive("/var/cache", 30, deleted_files_count);

        snprintf(message, sizeof(message), "Deleting tmp files from /var/log...");
        mvprintw(l++, 0, "%s", message);
        refresh();
        deleted_files_count = cleanup_log_files("/var/log", 365, deleted_files_count);
        
        sleep(1);

        clear();
        snprintf(message, sizeof(message), "Deleted %d tmp files.", deleted_files_count);
        mvprintw(LINES / 2, (COLS - strlen(message)) / 2, "%s", message);
        snprintf(message, sizeof(message), "details at /var/log/deleted_tmp_files.log");
        mvprintw(LINES / 2 + 1, (COLS - strlen(message)) / 2, "%s", message);
        refresh();
    }
    else
    {
        clear();
        snprintf(message, sizeof(message), "Press any key to exit");
        mvprintw(LINES / 2, (COLS - strlen(message)) / 2, "%s", message);
        refresh();
    }

    getch();
    endwin();
    return 0;
}
