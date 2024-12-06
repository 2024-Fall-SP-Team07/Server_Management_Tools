#include "h_for_tmp_cleanup.h"
#include "check_before_cleanup.h"
#include "cleanup.h"

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

int tmp_cleanup() {
    initscr();
    cbreak();
    curs_set(0);

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
    endwin();
}