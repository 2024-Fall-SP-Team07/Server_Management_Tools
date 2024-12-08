#include "log_viewer.h"
#include <ncurses.h>
#include <sys/ioctl.h>
#include <unistd.h>

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
    struct winsize w;

    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    max_y = w.ws_row;
    max_x = w.ws_col;

    WINDOW *main_win = newwin(max_y, max_x, 0, 0);
    box(main_win, 0, 0);
    refresh();

    display_message(main_win, login_attempts, login_failures);
    display_footer_message(main_win, max_y, max_x);

    const char *message = "Press Enter to see detailed logs.";
    int cursor_x = 2 + strlen(message);
    wmove(main_win, 2, cursor_x);
    wrefresh(main_win);

    int show_details = 0;

    while (1) {
        flushinp();
        int ch = wgetch(main_win);

        if (ch == '0') {
            break;
        } else if (ch == '\n') {
            show_details = 1;
        }

        if (show_details) {
            wclear(main_win);
            box(main_win, 0, 0);

            display_message(main_win, login_attempts, login_failures);
            display_title(main_win, max_x);

            int max_display_lines = max_y - 8;
            displayed_lines = read_log_file(main_win, "/var/log/wtmp", start_line, max_display_lines, &total_lines);

            display_more_message(main_win, max_y - 3, start_line + displayed_lines, total_lines);
            display_footer_message(main_win, max_y, max_x);

            wrefresh(main_win);

            ch = wgetch(main_win);

            if (ch == KEY_SPACE) {
                start_line += max_display_lines;
                if (start_line >= total_lines) {
                    start_line = 0;
                }

                wclear(main_win);
                box(main_win, 0, 0);

                display_message(main_win, login_attempts, login_failures);
                display_title(main_win, max_x);

                displayed_lines = read_log_file(main_win, "/var/log/wtmp", start_line, max_display_lines, &total_lines);

                display_more_message(main_win, max_y - 3, start_line + displayed_lines, total_lines);
                display_footer_message(main_win, max_y, max_x);

                wrefresh(main_win);
            }
        }
    }

    delwin(main_win);
    endwin();
    return 0;
}
