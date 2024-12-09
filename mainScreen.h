#ifndef MAINSCREEN_H

#define MAINSCREEN_H
#define MAX_MENU_ITEMS 6

typedef struct {
    const char *label;
    void (*action)();
} MenuItem;

void menu_action_1();
void menu_action_2();
void menu_action_3();
void menu_action_4();
void menu_action_5();
void menu_action_exit();
void display_menu(MenuItem menu[], int);

#endif