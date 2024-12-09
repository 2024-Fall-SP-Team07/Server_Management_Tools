#include "mainScreen.h"
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <termio.h>

int main() {
    initscr();              
    cbreak();               
    noecho();               
    keypad(stdscr, TRUE);   
    curs_set(0);

    MenuItem menu[MAX_MENU_ITEMS] = {
        {"1. Temporary_File_Cleaning", menu_action_1},
        {"2. Password_Checking", menu_action_2},
        {"3. Log_Checking", menu_action_3},
        {"4. Permission_Checking", menu_action_4},
        {"5. Resources Monitor", menu_action_5},
        {"Quit", menu_action_exit}  // Quit 메뉴 항목 추가
    };

    int current = 0; // 현재 선택된 메뉴 항목 인덱스
    int ch;

    while (1) {
        erase();
        display_menu(menu, current);
        ch = getch();
        switch (ch) {
            case KEY_UP:
                current = (current > 0) ? current - 1 : MAX_MENU_ITEMS - 1;
                break;
            case KEY_DOWN:
                current = (current < MAX_MENU_ITEMS - 1) ? current + 1 : 0;
                break;
            case 10: // Enter
                menu[current].action(); // 선택된 메뉴 항목의 동작 실행
                break;
        }
    }
    endwin(); // ncurses 종료
    return 0;
}


void menu_action_1() {
    printw("You selected Option 1.\n");
    getch();
}

void menu_action_2() {
    clear();
    printw("You selected Option 2.\n");
    refresh();
    getch();
}

void menu_action_3() {
    clear();
    printw("You selected Option 3.\n");
    refresh();
    getch();
}

void menu_action_4() {
    clear();
    printw("You selected Option 4.\n");
    refresh();
    getch();
}

void menu_action_5(){
    clear();
    printf("You selected Option 5.\n");
    refresh();
    getch();
}

void menu_action_exit() {
    clear();
    printw("Exiting menu...\n");
    refresh();
    endwin();
    exit(0); 
}

void display_menu(MenuItem menu[], int current) {
    struct winsize wbuf;
    const char *title = "System Management Program - Main Menu";
    int title_x, title_y, start_x, start_y;
    ioctl(0, TIOCGWINSZ, &wbuf);
    title_x = (wbuf.ws_col - strlen(title)) / 2;
    title_y = (wbuf.ws_row / 2) - 9;
    start_y = (wbuf.ws_row - MAX_MENU_ITEMS * 2) / 2;
    start_x = (wbuf.ws_col - strlen("1. Temporary_File_Cleaning  ")) / 2;

    mvprintw(title_y, title_x, "%s", title);

    for (int i = 0; i < MAX_MENU_ITEMS; i++) {
        if (i == current) {
            attron(A_REVERSE);
        }
        mvprintw(start_y + 2*i, start_x, "%s", menu[i].label);
        if (i == current) {
            attroff(A_REVERSE);
        }
    }
    refresh();
}
