#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define MAX_PATH 512
#define LOG_FILE "permissions_log.txt"

void run_command(const char *command) {
    int ret = system(command);
    if (ret != 0) {
        fprintf(stderr, "Error executing command: %s\n", command);
    }
}

// 텍스트 파일을 읽어 권한 설정
void process_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
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
        char path[MAX_PATH], owner[64], group[64], permissions[16];
        int immutable = 0;

        char *token = strtok(line, "|");
        if (!token) continue;
        strncpy(path, token, sizeof(path));
        path[strcspn(path, "\n")] = '\0';

        token = strtok(NULL, "&");
        if (!token) continue;

        sscanf(token, "%[^.].%[^,], %s", owner, group, permissions);

        if (strstr(line, "chattr +i")) {
            immutable = 1;
        }

        struct stat file_stat;
        if (stat(path, &file_stat) == -1) {
            fprintf(stderr, "Warning: File not found or inaccessible: %s\n", path);
            continue;
        }

        total_files++;
        int file_changed = 0;

        char command[MAX_PATH];
        snprintf(command, sizeof(command), "chown %s:%s %s", owner, group, path);
        if (system(command) == 0) {
            file_changed = 1;
        }

        snprintf(command, sizeof(command), "chmod %s %s", permissions, path);
        if (system(command) == 0) {
            file_changed = 1;
        }

        if (immutable) {
            snprintf(command, sizeof(command), "chattr +i %s", path);
        } else {
            snprintf(command, sizeof(command), "chattr -i %s", path);
        }
        if (system(command) == 0) {
            file_changed = 1;
        }

        // 로그 기록
        if (file_changed) {
            fprintf(log_file, "Modified: %s | Owner: %s:%s, Permissions: %s, Immutable: %s\n",
                    path, owner, group, permissions, immutable ? "Yes" : "No");
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
