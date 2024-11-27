#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>


void check_password_expiry(const char *username) {
    char command[256];
    snprintf(command, sizeof(command), "sudo chage -l %s", username);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("chage 명령어 실행 실패");
        return;
    }

    char buffer[256];
    printf("사용자 %s의 비밀번호 변경 주기 정보:\n", username);
    

    time_t now = time(NULL);
    struct tm last_change_tm = {0};
    int last_change_found = 0;
    char nowtime[256];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        
        if (strstr(buffer, "Last password change") != NULL) {
            printf("Last password change: %s", strchr(buffer, ':') + 2);
            strcpy(nowtime, strchr(buffer, ':') + 2);
            nowtime[strlen(nowtime) - 1] = '\0';
          
            // "Last password change" 날짜 파싱
            if (strptime(strchr(buffer, ':') + 2, "%b %d, %Y", &last_change_tm) != NULL) {
                last_change_found = 1;
            }

        }
    }

    // 마지막 비밀번호 변경 날짜와 현재 날짜의 차이 계산
    if (last_change_found) {
        time_t last_change_time = mktime(&last_change_tm);
        double days_diff = difftime(now, last_change_time) / (60 * 60 * 24);
        printf("%s(%d)일이 지났습니다.\n", nowtime, (int)days_diff);
    } else {
        printf("마지막 비밀번호 변경 날짜를 찾을 수 없습니다.\n");
    }

    printf("\n");
    pclose(fp);
}

void check_all_users_password_expiry() {
    FILE *fp = fopen("/etc/passwd", "r");
    if (fp == NULL) {
        perror("/etc/passwd 파일을 열 수 없습니다");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        char temp_line[256];
        strncpy(temp_line, line, sizeof(temp_line));
        temp_line[sizeof(temp_line) - 1] = '\0';
        char *username = strtok(temp_line, ":");
        if (username == NULL) {
            continue;
        }

        char *last_colon = strrchr(line, ':');
        if (last_colon && (strstr(last_colon, "bash") || strstr(last_colon, "sh") || strstr(last_colon, "zsh"))) {
            check_password_expiry(username);
        }
    }

    fclose(fp);
}

int main() {
    
    check_all_users_password_expiry();  // 비밀번호 만료 주기 확인

    return 0;
}


