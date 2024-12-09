#include <stdio.h>
#include <stdlib.h>
#include <utmp.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define KEY_SPACE 32

typedef struct {
    struct utmp log;
    int failure_count;
} LogEntry;

int compare_logs(const void *a, const void *b) {
    const LogEntry *entry_a = (const LogEntry *)a;
    const LogEntry *entry_b = (const LogEntry *)b;
    return entry_b->log.ut_tv.tv_sec - entry_a->log.ut_tv.tv_sec;
}

void calculate_log_stats(const char *wtmp_file, const char *btmp_file, int *login_attempts, int *login_failures) {
    FILE *fp;
    struct utmp log;

    *login_attempts = 0;
    *login_failures = 0;

    fp = fopen(wtmp_file, "rb");
    if (fp != NULL) {
        while (fread(&log, sizeof(struct utmp), 1, fp) == 1) {
            if (log.ut_type == USER_PROCESS) {
                (*login_attempts)++;
            }
        }
        fclose(fp);
    }

    fp = fopen(btmp_file, "rb");
    if (fp != NULL) {
        while (fread(&log, sizeof(struct utmp), 1, fp) == 1) {
            if (log.ut_type == LOGIN_PROCESS || log.ut_type == USER_PROCESS) {
                (*login_failures)++;
            }
        }
        fclose(fp);
    }
}

void print_log_info(WINDOW *win, struct utmp *log, int failure_count, int y, int x) {
    char date_str[10];
    char time_str[10];
    time_t login_time = log->ut_tv.tv_sec;
    struct tm *time_info = localtime(&login_time);

    strftime(date_str, sizeof(date_str), "%y/%m/%d", time_info);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", time_info);

    char ip_info[256];
    if (strlen(log->ut_host) > 0) {
        snprintf(ip_info, sizeof(ip_info), "%s", log->ut_host);
    } else {
        snprintf(ip_info, sizeof(ip_info), "N/A");
    }

    mvwprintw(win, y, x, "%-20s | %-10s | %-8s | %-15s | %-11d",
              log->ut_user, date_str, time_str, ip_info, failure_count);
}

int read_log_file(WINDOW *win, const char *file_path, int start_line, int max_lines, int *total_lines) {
    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL) {
        perror("Error opening file");
        return -1;
    }

    struct utmp log;
    LogEntry *logs = NULL;
    int log_count = 0;
    int failure_count = 0;

    while (fread(&log, sizeof(struct utmp), 1, fp) == 1) {
        if (log.ut_type == USER_PROCESS || log.ut_type == DEAD_PROCESS) {
            logs = realloc(logs, sizeof(LogEntry) * (log_count + 1));
            logs[log_count].log = log;
            logs[log_count].failure_count = failure_count;
            log_count++;
        } else if (log.ut_type == LOGIN_PROCESS) {
            failure_count++;
        }
    }
    fclose(fp);

    qsort(logs, log_count, sizeof(LogEntry), compare_logs);

    *total_lines = log_count;

    int displayed = 0;
    for (int i = start_line; i < log_count && displayed < max_lines; i++) {
        print_log_info(win, &logs[i].log, logs[i].failure_count, displayed + 6, 2);
        displayed++;
    }

    free(logs);
    return displayed;
}

void display_message(WINDOW *win, int login_attempts, int login_failures) {
    mvwprintw(win, 1, 2, "# of login attempts: %d   # of login failures: %d",
              login_attempts, login_failures);
}

void display_title(WINDOW *win, int max_x) {
    const char *title = "Username             | Last login | Attempt  | IP              | # of failures";
    int line_length = max_x - 4;
    char line[line_length + 1];
    memset(line, '-', line_length);
    line[line_length] = '\0';

    mvwprintw(win, 3, 2, "%s", line);
    mvwprintw(win, 4, 2, "%s", title);
    mvwprintw(win, 5, 2, "%s", line);
}

void display_footer_message(WINDOW *win, int max_y, int max_x) {
    const char *footer = "To restore main screen, Press \"q\"";
    mvwprintw(win, max_y - 2, (max_x - strlen(footer)) / 2, "%s", footer);
}

void display_more_message(WINDOW *win, int y, int current, int total) {
    const char *message = " more? ";
    const char *hint = "[space-next, 0-exit]";

    wattron(win, A_REVERSE);
    mvwprintw(win, y, 2, "%s", message);
    wattroff(win, A_REVERSE);

    mvwprintw(win, y, 8, "(%d/%d) %s", current, total, hint);
}

void display_press_enter_message(WINDOW *win) {
    const char *message = "Press ";
    const char *highlight = "Enter";
    const char *suffix = " to see detailed logs.";

    mvwprintw(win, 2, 2, "%s", message);
    wattron(win, A_REVERSE);
    wprintw(win, "%s", highlight);
    wattroff(win, A_REVERSE);
    wprintw(win, "%s", suffix);
}

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int login_attempts, login_failures;
    calculate_log_stats("/var/log/wtmp", "/var/log/btmp", &login_attempts, &login_failures);

    int max_y, max_x;
    int start_line = 0;
    int total_lines = 0;
    int displayed_lines = 0;
    int show_logs = 0; 
    struct winsize w;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    max_y = w.ws_row;
    max_x = w.ws_col;

    WINDOW *main_win = newwin(max_y, max_x, 0, 0);
    refresh();

    display_message(main_win, login_attempts, login_failures);
    if (!show_logs) {
        display_press_enter_message(main_win);
    }
    display_footer_message(main_win, max_y, max_x); 
    wrefresh(main_win);

    while (1) {
        wclear(main_win);

        if (!show_logs) {
            display_message(main_win, login_attempts, login_failures);
            display_press_enter_message(main_win);
        } else {
            display_message(main_win, login_attempts, login_failures);
            display_title(main_win, max_x);

            int max_display_lines = max_y - 9; 
            displayed_lines = read_log_file(main_win, "/var/log/wtmp", start_line, max_display_lines, &total_lines);

            if (displayed_lines < max_display_lines) {
                display_footer_message(main_win, max_y, max_x);
            } else {
                display_more_message(main_win, max_y - 3, start_line + displayed_lines, total_lines);
            }
        }

        display_footer_message(main_win, max_y, max_x); 
        wrefresh(main_win);

        int ch = wgetch(main_win);

        if (ch == '0') {
            break;
        } else if (ch == '\n') {
            show_logs = 1; 
        } else if (ch == KEY_SPACE && show_logs) {
            start_line += displayed_lines;
            if (start_line >= total_lines) {
                start_line = 0;
            }
        }
    }

    delwin(main_win);
    endwin();
    return 0;
}
