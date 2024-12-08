/*
/tmp, /var/tmp, /var/cache에서 조건에 따른 파일 삭제 구현 완료
시간 경과, 프로그램의 실행 및 잠금 여부, uid 및 gid 확인
/var/log에서 조건에 따른 파일 삭제 구현 완료
형식 확인, 시간 경과, 프로그램의 실행 및 잠금 여부
/dev파일은 /dev/null같은 파일의 경우 major, minor number가 0일 수 있고 아예 없는 경우를 찾는법을 찾지 못해 미구현

ncurses를 사용한 임시 파일의 삭제 여부확인, 삭제 진행도 표시, 삭제 완료 표시 구현 완료
*/
#define _XOPEN_SOURCE 700
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ncurses.h>

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


#define MAX_OUTPUT_LEN 512
#define MAX_USERS 100
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

int tmpclean() {
    int ch;
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
        mvprintw(LINES / 2 - 1, (COLS - strlen(message)) / 2, "%s", message);
        snprintf(message, sizeof(message), "details at /var/log/deleted_tmp_files.log");
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



void check_password_expiry(const char *username, char *output) {
   char command[256];
   snprintf(command, sizeof(command), "sudo chage -l %s", username);

   FILE *fp = popen(command, "r");
   if (fp == NULL) {
       snprintf(output, MAX_OUTPUT_LEN, "Failed to get %s user's information.\n", username);
       return;
   }

   char buffer[256];
   time_t now = time(NULL);
   struct tm last_change_tm = {0};
   int last_change_found = 0;
   char last_change_date[256] = "N/A";

   while (fgets(buffer, sizeof(buffer), fp) != NULL) {
       if (strstr(buffer, "Last password change") != NULL) {
           strncpy(last_change_date, strchr(buffer, ':') + 2, sizeof(last_change_date));
           last_change_date[strlen(last_change_date) - 1] = '\0';  // Remove newline
           if (strptime(last_change_date, "%b %d, %Y", &last_change_tm) != NULL) {
               last_change_found = 1;
           }
       }
   }

   if (last_change_found) {
       time_t last_change_time = mktime(&last_change_tm);
       double days_diff = difftime(now, last_change_time) / (60 * 60 * 24);
       snprintf(output, MAX_OUTPUT_LEN, "User: %s, Last change: %s (%d days ago)\n",
                username, last_change_date, (int)days_diff);
   } else {
       snprintf(output, MAX_OUTPUT_LEN, "User: %s, Last change: %s\n", username, "No information");
   }

   pclose(fp);
}

// Function to get all users with shell access from /etc/passwd
int get_all_users(char users[MAX_USERS][256], int *total_users) {
   FILE *fp = fopen("/etc/passwd", "r");
   if (fp == NULL) {
       perror("/etc/passwd file cannot be opened");
       return -1;
   }

   char line[256];
   int user_count = 0;
   while (fgets(line, sizeof(line), fp) != NULL && user_count < MAX_USERS) {
       char temp_line[256];
       strncpy(temp_line, line, sizeof(temp_line));
       temp_line[sizeof(temp_line) - 1] = '\0';
       char *username = strtok(temp_line, ":");
       if (username == NULL) {
           continue;
       }

       char *last_colon = strrchr(line, ':');
       if (last_colon && (strstr(last_colon, "bash") || strstr(last_colon, "sh") || strstr(last_colon, "zsh"))) {
           strncpy(users[user_count], username, 256);
           users[user_count][255] = '\0';
           user_count++;
       }
   }

   fclose(fp);
   *total_users = user_count;
   return 0;
}

// Function to display user information with ncurses and pagination
void display_users_with_ncurses(const char users[MAX_USERS][256], int total_users) {
   int start_index = 0;
   char output[MAX_OUTPUT_LEN];

   initscr();              // Initialize ncurses
   cbreak();               // Immediate input processing
   noecho();               // Do not echo user input
   keypad(stdscr, TRUE);   // Enable arrow keys

   while (1) {
       clear(); // Clear screen

       // Get terminal size
       int rows, cols;
       getmaxyx(stdscr, rows, cols);

       int lines_per_page = rows - 4; // Exclude header/footer rows
       int end_index = start_index + lines_per_page;
       if (end_index > total_users) {
           end_index = total_users;
       }

       mvprintw(0, 0, "Displaying users %d to %d (Total: %d)", start_index + 1, end_index, total_users);
       mvprintw(1, 0, "[Enter/Space: Next page, q: Quit]\n");

       // Display user password information
       for (int i = start_index; i < end_index; i++) {
           check_password_expiry(users[i], output);
           mvprintw(3 + (i - start_index), 0, "%s", output);
       }

       refresh();

       // Handle user input
       int ch = getch();
       if (ch == 'q') {
           break; // Quit program
       } else if (ch == ' ' || ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
           if (end_index < total_users) {
               start_index += lines_per_page; // Move to the next page
           } else {
               mvprintw(rows - 1, 0, "End of list. Press 'q' to quit.");
               refresh();
               getch(); // Wait for input
               break;
           }
       }
   }

   endwin(); // End ncurses mode
}

// New function that acts like 'main'
void run_program() {
   char users[MAX_USERS][256];
   int total_users = 0;

   // Get all user information
   if (get_all_users(users, &total_users) == 0) {
       // Display user information with ncurses
       display_users_with_ncurses(users, total_users);
   } else {
       printf("Failed to get user information.\n");
   }
}




#define MAX_MENU_ITEMS 6



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

void menu_action_2() {
    clear();
    printw("You selected Option 2.\n");
    run_program();
    refresh();
    getch(); // 동작 후 키 입력 대기
}

void menu_action_3() {
    clear();
    printw("You selected Option 3.\n");
    refresh();
    getch(); // 동작 후 키 입력 대기
}

void menu_action_4() {
    clear();
    printw("You selected Option 4.\n");
    refresh();
    getch(); // 동작 후 키 입력 대기
}

void menu_action_5(){
    clear();
    printf("You selected Option 5.\n");
    refresh();
    getch();
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
    /*
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
    refresh();*/
    const char *title="System management Main menu";
    int title_x= (COLS-strlen(title))/2;
    mvprintw(1,title_x,"%s",title);
     int menu_height = MAX_MENU_ITEMS; // 메뉴 항목의 개수
    int start_y = (LINES - menu_height) / 2; // 화면 세로 중앙 계산
    int start_x = (COLS - 20) / 2;  // 화면 가로 중앙 계산 (메뉴 가로 길이 20 기준)

    for (int i = 0; i < MAX_MENU_ITEMS; i++) {
        if (i == current) {
            attron(A_REVERSE); // 현재 선택된 항목은 반전 효과
        }
        mvprintw(start_y + i, start_x, "%s", menu[i].label); // 중앙에 메뉴 출력
        if (i == current) {
            attroff(A_REVERSE); // 반전 효과 종료
        }
    }
    refresh();


}


// Function to check password expiry information for a user






int main() {
    initscr();              // ncurses 초기화
    cbreak();               // 입력을 한 문자씩 받음
    noecho();               // 입력 문자 화면에 표시 안 함
    keypad(stdscr, TRUE);   // 방향키 사용 가능
    curs_set(0);            // 커서 숨김

    MenuItem menu[MAX_MENU_ITEMS] = {
        {"1. Temporary_File_Cleaning", menu_action_1},
        {"2. Password_Checking", menu_action_2},
        {"3. Log_Checking", menu_action_3},
        {"4. Permission_Checking", menu_action_4},
        {"5. Resources Monitor",menu_action_5},
        {"Quit", menu_action_exit}  // Quit 메뉴 항목 추가
    };

    int current = 0; // 현재 선택된 메뉴 항목 인덱스
    int ch;

    while (1) {
        erase();
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
