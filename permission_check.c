#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <time.h>
#include <ncurses.h>

#define MAX_LINE_LENGTH_A 512
#define MAX_LINE_LENGTH 256
#define MAX_ITEMS_PER_PAGE 10
#define LOG_FILE_PREFIX "permission_fix_log_"

typedef struct {
    char filename[MAX_LINE_LENGTH];
    char path[MAX_LINE_LENGTH];
    char old_owner[MAX_LINE_LENGTH_A];
    char new_owner[MAX_LINE_LENGTH_A];
    char old_perms[16];
    char new_perms[16];
} FileLog;

typedef struct {
    char path[MAX_LINE_LENGTH];
    char owner[MAX_LINE_LENGTH];
    char group[MAX_LINE_LENGTH];
    int perms;
} FilePermission;

void get_current_time(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y%m%d_%H%M%S", t);
}

void truncate_string(const char *input, char *output, size_t max_length) {
    if (strlen(input) > max_length) {
        snprintf(output, max_length + 1, "%.*s...", (int)(max_length - 3), input);
    } else {
        strcpy(output, input);
    }
}

int parse_permissions(const char *filename, FilePermission **permissions, int *count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file_permissions.txt");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    int i = 0;

    while (fgets(line, sizeof(line), file)) {
        FilePermission temp;
        char owner_group[MAX_LINE_LENGTH];
        if (sscanf(line, "%[^|]|%[^,],%o", temp.path, owner_group, &temp.perms) == 3) {
            if (sscanf(owner_group, "%[^.].%s", temp.owner, temp.group) != 2) {
                fprintf(stderr, "Invalid owner.group format: %s\n", owner_group);
                continue;
            }

            FilePermission *new_permissions = realloc(*permissions, sizeof(FilePermission) * (i + 1));
            if (!new_permissions) {
                perror("Memory allocation failed");
                free(*permissions);
                fclose(file);
                return -1;
            }

            *permissions = new_permissions;
            (*permissions)[i++] = temp;
        } else {
            fprintf(stderr, "Invalid line format: %s\n", line);
        }
    }

    fclose(file);
    *count = i;
    return 0;
}

int check_and_fix_permissions(FilePermission *permissions, int count, FileLog **logs, int *log_count, char *log_file) {
    FILE *log = fopen(log_file, "w");
    if (!log) {
        perror("Failed to open log file");
        return -1;
    }

    int invalid_files = 0;

    for (int i = 0; i < count; i++) {
        struct stat st;
        if (stat(permissions[i].path, &st) == -1) {
            fprintf(stderr, "[WARNING] %s does not exist.\n", permissions[i].path);
            continue;
        }

        struct passwd *pwd = getpwuid(st.st_uid);
        struct group *grp = getgrgid(st.st_gid);
        int current_perms = st.st_mode & 0777;

        if (!pwd || !grp) {
            fprintf(stderr, "[ERROR] Failed to get owner/group for %s.\n", permissions[i].path);
            continue;
        }

        if (current_perms != permissions[i].perms ||
            strcmp(pwd->pw_name, permissions[i].owner) != 0 ||
            strcmp(grp->gr_name, permissions[i].group) != 0) {

            struct passwd *new_pwd = getpwnam(permissions[i].owner);
            struct group *new_grp = getgrnam(permissions[i].group);

            if (!new_pwd || !new_grp) {
                fprintf(stderr, "[ERROR] Invalid owner or group for %s: %s.%s\n",
                        permissions[i].path, permissions[i].owner, permissions[i].group);
                continue;
            }

            uid_t uid = new_pwd->pw_uid;
            gid_t gid = new_grp->gr_gid;

            chown(permissions[i].path, uid, gid);
            chmod(permissions[i].path, permissions[i].perms);

            FileLog log_entry;
            const char *basename = strrchr(permissions[i].path, '/');
            if (basename) {
                snprintf(log_entry.filename, sizeof(log_entry.filename), "%s", basename + 1);
            } else {
                snprintf(log_entry.filename, sizeof(log_entry.filename), "%s", permissions[i].path);
            }
            snprintf(log_entry.path, sizeof(log_entry.path), "%s", permissions[i].path);
            snprintf(log_entry.old_owner, sizeof(log_entry.old_owner), "%s.%s", pwd->pw_name, grp->gr_name);
            snprintf(log_entry.new_owner, sizeof(log_entry.new_owner), "%s.%s", permissions[i].owner, permissions[i].group);
            snprintf(log_entry.old_perms, sizeof(log_entry.old_perms), "%o", current_perms);
            snprintf(log_entry.new_perms, sizeof(log_entry.new_perms), "%o", permissions[i].perms);

            FileLog *new_logs = realloc(*logs, sizeof(FileLog) * (*log_count + 1));
            if (!new_logs) {
                perror("Memory allocation failed");
                free(*logs);
                fclose(log);
                return -1;
            }
            *logs = new_logs;
            (*logs)[(*log_count)++] = log_entry;

            fprintf(log, "%s | %s.%s,%o -> %s.%s,%o\n",
                    permissions[i].path,
                    pwd->pw_name, grp->gr_name, current_perms,
                    permissions[i].owner, permissions[i].group, permissions[i].perms);

            invalid_files++;
        }
    }

    fclose(log);
    return invalid_files;
}

void display_page(FileLog *logs, int total_files, int invalid_files, const char *log_file, int count, int page) {
    clear();
    mvprintw(0, 0, "# of total files: %d | # of invalid files: %d", total_files, invalid_files);
    mvprintw(1, 0, "Detail data is saved to log file: %s", log_file);
    mvprintw(2, 0, "------------------------------------------------------------------------------------------------------");
    mvprintw(3, 0, "File Name      Path             Owner.Group(Priv->Cur)                       Permission(Priv->Cur)");
    mvprintw(4, 0, "------------------------------------------------------------------------------------------------------");

    int start = page * MAX_ITEMS_PER_PAGE;
    int end = (start + MAX_ITEMS_PER_PAGE < count) ? start + MAX_ITEMS_PER_PAGE : count;

    char truncated_filename[16];
    char truncated_path[18];
    char truncated_old_owner[22];
    char truncated_new_owner[22];

    for (int i = start, row = 5; i < end; i++, row++) {
        truncate_string(logs[i].filename, truncated_filename, 14);
        truncate_string(logs[i].path, truncated_path, 16);
        truncate_string(logs[i].old_owner, truncated_old_owner, 20);
        truncate_string(logs[i].new_owner, truncated_new_owner, 20);

        mvprintw(row, 0, "%-14s %-16s %-20s -> %-20s %-14s -> %-14s",
                 truncated_filename,
                 truncated_path,
                 truncated_old_owner, truncated_new_owner,
                 logs[i].old_perms, logs[i].new_perms);
    }

    if (end < count) {
        mvprintw(LINES - 1, 0, "more? (Press 'n' for next page, 'q' to quit)");
    } else {
        mvprintw(LINES - 1, 0, "End of results. (Press 'q' to quit)");
    }

    refresh();
}

void ncurses_display(FileLog *logs, int total_files, int invalid_files, const char *log_file, int count) {
    initscr();
    noecho();
    cbreak();

    int page = 0;
    int ch;

    while (1) {
        display_page(logs, total_files, invalid_files, log_file, count, page);

        ch = getch();
        if (ch == 'q') {
            break;
        } else if (ch == 'n' && (page + 1) * MAX_ITEMS_PER_PAGE < count) {
            page++;
        } else if (ch == 'p' && page > 0) {
            page--;
        }
    }

    endwin();
}

int main() {
    const char *input_file = "file_permissions.txt";
    char log_file[MAX_LINE_LENGTH];
    FilePermission *permissions = NULL;
    FileLog *logs = NULL;
    int count = 0, log_count = 0;

    //파일이 없을 때 -> 파일이 없음을 출력하고 q키를 입력하면 종료
    struct stat file_stat;
    if (stat(input_file, &file_stat) == -1) {
        initscr();
        noecho();
        cbreak();

        mvprintw(0, 0, "Error: file_permissions.txt does not exist.");
        mvprintw(1, 0, "Press 'q' to quit.");
        refresh();

        while (1) {
            int ch = getch();
            if (ch == 'q') {
                endwin();
                return EXIT_FAILURE;
            }
        }
    }

    if (parse_permissions(input_file, &permissions, &count) == -1) {
        return EXIT_FAILURE;
    }

    char timestamp[64];
    get_current_time(timestamp, sizeof(timestamp));
    snprintf(log_file, sizeof(log_file), "/var/log/00_Server_Management/%s%s.txt", LOG_FILE_PREFIX, timestamp);

    struct stat st;
    if (stat("/var/log/00_Server_Management/", &st) == -1) {
        perror("Log directory does not exist. Please create it.");
        free(permissions);
        return EXIT_FAILURE;
    }

    int invalid_files = check_and_fix_permissions(permissions, count, &logs, &log_count, log_file);

    ncurses_display(logs, count, invalid_files, log_file, log_count);

    free(permissions);
    free(logs);
    return EXIT_SUCCESS;
}
