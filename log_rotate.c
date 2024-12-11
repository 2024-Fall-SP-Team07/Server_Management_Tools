#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ncurses.h>

typedef struct {
    char log_path[256];    // Log file path
    int max_size_mb;       // Max file size in MB
    int keep_days;         // Retention days
} LogConfig;

void generate_log_filename(const char *base_name, int index, char *out_filename) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);

    char date_str[16];
    strftime(date_str, sizeof(date_str), "%Y%m%d", tm_info);

    if (index >= 0) {
        snprintf(out_filename, 256, "%s_%s_%d.log", base_name, date_str, index);
    } else {
        snprintf(out_filename, 256, "%s_%s.log", base_name, date_str);
    }
}

void delete_old_logs(const char *log_dir, int keep_days) {
    char command[512];
    snprintf(command, sizeof(command), "find %s -name '*.log' -mtime +%d -exec rm -f {} \\;", log_dir, keep_days);
    int ret = system(command);

    if (ret == 0) {
        mvprintw(4, 0, "Log deletion completed successfully!");
    } else {
        mvprintw(4, 0, "Log deletion failed.");
    }
    refresh();
}

void rotate_log(LogConfig *config) {
    struct stat st;
    if (stat(config->log_path, &st) != 0) {
        mvprintw(3, 0, "Error accessing log file: %s", config->log_path);
        refresh();
        return;
    }

    int size_mb = st.st_size / (1024 * 1024);

    if (size_mb > config->max_size_mb) {
        int index = 0;
        char new_filename[256];
        generate_log_filename(config->log_path, index, new_filename);

        while (access(new_filename, F_OK) == 0) {
            index++;
            generate_log_filename(config->log_path, index, new_filename);
        }

        if (rename(config->log_path, new_filename) == 0) {
            fclose(fopen(config->log_path, "w"));
            mvprintw(3, 0, "Log rotation completed successfully!");
        } else {
            mvprintw(3, 0, "Log rotation failed.");
        }
    } else {
        mvprintw(3, 0, "Log rotation not required.");
    }
    refresh();
}

void draw_ui(LogConfig *config, char *log_dir) {
    initscr();
    noecho();
    cbreak();

    int y = 0;

    mvprintw(y++, 0, "<Log Rotation>");

    mvprintw(y, 0, "Enter the ");
    attron(A_REVERSE);
    printw("full path");
    attroff(A_REVERSE);
    printw(" of the log file: ");
    echo();
    move(y, 36); 
    getstr(config->log_path);
    noecho();
    y++;

    mvprintw(y, 0, "Enter the ");
    attron(A_REVERSE);
    printw("maximum size");
    attroff(A_REVERSE);
    printw(" for log rotation (MB): ");
    echo();
    move(y, 45); 
    scanw("%d", &config->max_size_mb);
    noecho();
    y++;

    mvprintw(y, 0, "Enter the ");
    attron(A_REVERSE);
    printw("directory");
    attroff(A_REVERSE);
    printw(" containing logs for ");
    attron(A_REVERSE);
    printw("deletion");
    attroff(A_REVERSE);
    printw(": ");
    echo();
    move(y, 49); 
    getstr(log_dir);
    noecho();
    y++;

    mvprintw(y, 0, "Enter the number of ");
    attron(A_REVERSE);
    printw("days");
    attroff(A_REVERSE);
    printw(" to keep old logs: ");
    echo();
    move(y, 42);
    scanw("%d", &config->keep_days);
    noecho();
    y++;

    clear();
    mvprintw(0, 0, "Processing log rotation and cleanup...");
    refresh();

    rotate_log(config);
    delete_old_logs(log_dir, config->keep_days);

    mvprintw(6, 0, "Press any key to exit.");
    refresh();

    getch();
    endwin();
}

int main() {
    LogConfig config;
    char log_dir[256];

    draw_ui(&config, log_dir);

    return 0;
}
ㄴ