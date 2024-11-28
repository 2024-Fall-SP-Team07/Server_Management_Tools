#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define LOG_FILE "permissions_log.txt"
#define MAX_PATH 1024
#define MAX_COMMAND 1024

void run_command(const char *command) {
    char silent_command[MAX_COMMAND];
    snprintf(silent_command, MAX_COMMAND, "%s > /dev/null 2>&1", command);
    if (system(silent_command) != 0) {
        fprintf(stderr, "Error executing: %s\n", command);
    }
}

void expand_path(const char *path, char *expanded) {
    const char *home = getenv("HOME");
    const char *user = getenv("USER");
    if (!home || !user) {
        fprintf(stderr, "Environment variables USER or HOME are not set.\n");
        exit(EXIT_FAILURE);
    }

    strncpy(expanded, path, MAX_PATH);

    char *username_pos = strstr(expanded, "${USERNAME}");
    if (username_pos) {
        char buffer[MAX_PATH];
        snprintf(buffer, sizeof(buffer), "%.*s%s%s", (int)(username_pos - expanded), expanded, user, username_pos + 11);
        strncpy(expanded, buffer, MAX_PATH);
    }

    char *home_pos = strstr(expanded, "$HOME");
    if (home_pos) {
        char buffer[MAX_PATH];
        snprintf(buffer, sizeof(buffer), "%.*s%s%s", (int)(home_pos - expanded), expanded, home, home_pos + 5);
        strncpy(expanded, buffer, MAX_PATH);
    }
}

// 텍스트 파일을 읽어 권한 설정
void process_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open input file");
        exit(EXIT_FAILURE);
    }

    FILE *log_file = fopen(LOG_FILE, "w");
    if (!log_file) {
        perror("Failed to open log file");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    char line[512];
    int total_files = 0, improper_files = 0;

    while (fgets(line, sizeof(line), file)) {
        char path[MAX_PATH], expanded_path[MAX_PATH];
        char owner[64], group[64], permissions[16];
        int immutable = 0;

        char *token = strtok(line, "|");
        if (!token) continue;
        strncpy(path, token, sizeof(path));

        token = strtok(NULL, "&");
        if (!token) continue;
        sscanf(token, "%[^.].%[^,], %s", owner, group, permissions);

        if (strstr(line, "chattr +i")) {
            immutable = 1;
        }

        expand_path(path, expanded_path);

        if (access(expanded_path, F_OK) != 0) {
            fprintf(log_file, "Warning: File not found: %s\n", expanded_path);
            continue;
        }

        total_files++;
        int file_changed = 0;


        char command[MAX_COMMAND];
        snprintf(command, sizeof(command), "chown %s:%s %s", owner, group, expanded_path);
        run_command(command);
        file_changed = 1;

        snprintf(command, sizeof(command), "chmod %s %s", permissions, expanded_path);
        run_command(command);
        file_changed = 1;

        if (immutable) {
            snprintf(command, sizeof(command), "chattr +i %s", expanded_path);
        } else {
            snprintf(command, sizeof(command), "chattr -i %s", expanded_path);
        }
        run_command(command);
        file_changed = 1;

        // 로그 작성
        if (file_changed) {
            fprintf(log_file, "Modified: %s | Owner: %s:%s, Permissions: %s, Immutable: %s\n",
                    expanded_path, owner, group, permissions, immutable ? "Yes" : "No");
            improper_files++;
        }
    }

    fclose(file);
    fclose(log_file);

    printf("Total files checked: %d\n", total_files);
    printf("Improper files fixed: %d\n", improper_files);
    printf("Log file saved at: %s\n", LOG_FILE);
}

int main() {
    const char *filename = "file_permissions.txt";
    process_file(filename);
    return 0;
}