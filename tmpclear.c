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
    int log_fd = open("/var/log/00_Server_Management/deleted_tmp_files.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
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

int cleanup_files_recursive(const char *dirpath, int max_age_days, int deleted_count, int line) {
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
            deleted_count = cleanup_files_recursive(filepath, max_age_days, deleted_count, line);
            if (is_directory_empty(filepath)) {
                int is_old_enough = file_age_check(filepath, max_age_days);
                int is_invalid_owner_group = !is_valid_owner_group(filepath);
                if (is_old_enough || is_invalid_owner_group) {
                    if (rmdir(filepath) == -1) {
                        // perror("rmdir failed");
                    } else {
                        log_deletion_record(filepath);
                        deleted_count++; // 파일 삭제 시 카운트 증가
                        refresh();
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
                    refresh();
                }
            }
        }
    }
    closedir(dir);
    return deleted_count;
}

int cleanup_log_files(const char *log_dir, int max_age_days, int deleted_count, int line) {
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
        char *date_part = strrchr(entry->d_name, '-');  // 파일 이름에서 마지막 '-'을 찾음
        if (date_part != NULL) {
        date_part++;  // '-' 뒤의 날짜 부분으로 이동 (날짜는 '-' 뒤에 있음)
        if (strlen(date_part) == 8) {  // 날짜가 8자리여야 함 (YYYYMMDD)
            int year, month, day;
            // 날짜를 YYYYMMDD 형식으로 파싱
            if (sscanf(date_part, "%4d%2d%2d", &year, &month, &day) == 3) {
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
                            refresh();
                        }
                    } else {
                        snprintf(filepath, sizeof(filepath), "%s/%s", log_dir, entry->d_name);
                        if (is_file_in_use(filepath) || is_file_locked(filepath)) {
                            continue;
                        }
                        else {
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
                                if (remove(filepath) == -1) {
                                // perror("remove failed");
                                } else {
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

int tmpclean() {
    int ch;
    int l = 0;
    int delete_files = ask_delete_confirmation(&l);
    int tmp_deleted_files_count = 0; // 삭제된 파일 개수 추적
    int var_tmp_deleted_files_count = 0;
    int var_cache_deleted_files_count = 0;
    int var_log_deleted_files_count = 0;
    int total_deleted_files = 0;

    char message[128];

    if (delete_files) {
        snprintf(message, sizeof(message), "Deleting tmp files from /tmp...");
        mvprintw(l, 0, "%s", message);
        refresh();
        tmp_deleted_files_count = cleanup_files_recursive("/tmp", 1, tmp_deleted_files_count, l++);
        mvprintw(l - 1, 42, "number of deleted file : %d", tmp_deleted_files_count);

        snprintf(message, sizeof(message), "Deleting tmp files from /var/tmp...");
        mvprintw(l, 0, "%s", message);
        refresh();
        var_tmp_deleted_files_count = cleanup_files_recursive("/var/tmp", 7, var_tmp_deleted_files_count, l++);
        mvprintw(l - 1, 42, "number of deleted file : %d", var_tmp_deleted_files_count);
        
        snprintf(message, sizeof(message), "Deleting tmp files from /var/cache...");
        mvprintw(l, 0, "%s", message);
        refresh();
        var_cache_deleted_files_count = cleanup_files_recursive("/var/cache", 30, var_cache_deleted_files_count, l++);
        mvprintw(l - 1, 42, "number of deleted file : %d", var_cache_deleted_files_count);

        snprintf(message, sizeof(message), "Deleting tmp files from /var/log...");
        mvprintw(l, 0, "%s", message);
        refresh();
        var_log_deleted_files_count = cleanup_log_files("/var/log", 365, var_log_deleted_files_count, l++);
        mvprintw(l - 1, 42, "number of deleted file : %d", var_log_deleted_files_count);
        
        mvprintw(l, 0, "Enter any key to see result");
        getch();

        clear();
        total_deleted_files = tmp_deleted_files_count + var_tmp_deleted_files_count + var_cache_deleted_files_count + var_log_deleted_files_count;
        snprintf(message, sizeof(message), "Deleted %d tmp files.", total_deleted_files);
        mvprintw(LINES / 2 - 1, (COLS - strlen(message)) / 2, "%s", message);
        snprintf(message, sizeof(message), "details at /var/log/00_Server_Management/deleted_tmp_files.log");
        mvprintw(LINES / 2, (COLS - strlen(message)) / 2, "%s", message);
        snprintf(message, sizeof(message), "To restore main screen, Press \"q\"");
        mvprintw(LINES / 2 + 1, (COLS - strlen(message)) / 2, "%s", message);
        refresh();
    }
    else
    {
        clear();
        snprintf(message, sizeof(message), "To restore main screen, Press \"q\"");
        mvprintw(LINES / 2, (COLS - strlen(message)) / 2, "%s", message);
        refresh();
    }

    while ((ch = getch()) != 'q') 
    {
        getch();
    }

    return 0;
}

#define MAX_MENU_ITEMS 2

// 메뉴 항목에 대한 정보를 담고 있는 구조체
typedef struct {
    const char *label;   // 메뉴 항목 텍스트
    void (*action)();    // 해당 메뉴를 선택했을 때 실행될 함수
} MenuItem;

// 각 메뉴 항목에 대한 동작 정의
void menu_action_1() {
    tmpclean();
    getch(); // 동작 후 키 입력 대기
}

// 메뉴 항목 "Quit" 선택 시 프로그램 종료 처리 함수
void menu_action_exit() {
    clear();
    printw("Exiting menu...\n");
    refresh(); // 동작 후 키 입력 대기
    endwin(); // ncurses 종료
    exit(0); // 프로그램 종료
}

// 메뉴 UI를 출력하는 함수
void display_menu(MenuItem menu[], int current) {
    clear(); // 화면을 지운다

    for (int i = 0; i < MAX_MENU_ITEMS; i++) {
        if (i == current) {
            attron(A_REVERSE); // 현재 선택된 항목은 반전 효과
        }
        mvprintw(i, 0, "%s", menu[i].label);
        if (i == current) {
            attroff(A_REVERSE); // 반전 효과 종료
        }
    }
    refresh();
}

int main() {
    initscr();              // ncurses 초기화
    cbreak();               // 입력을 한 문자씩 받음
    noecho();               // 입력 문자 화면에 표시 안 함
    keypad(stdscr, TRUE);   // 방향키 사용 가능
    curs_set(0);            // 커서 숨김

    MenuItem menu[MAX_MENU_ITEMS] = {
        {"Option 1", menu_action_1},
        {"Quit", menu_action_exit}  // Quit 메뉴 항목 추가
    };

    int current = 0; // 현재 선택된 메뉴 항목 인덱스
    int ch;

    while (1) {
        display_menu(menu, current); // 메뉴를 화면에 표시

        ch = getch(); // 사용자 입력 받기
        switch (ch) {
            case KEY_UP:
                current = (current > 0) ? current - 1 : MAX_MENU_ITEMS - 1;
                break;
            case KEY_DOWN:
                current = (current < MAX_MENU_ITEMS - 1) ? current + 1 : 0;
                break;
            case 10: // Enter 키
                menu[current].action(); // 선택된 메뉴 항목의 동작 실행
                break;
        }
    }

    endwin(); // ncurses 종료
    return 0;
}