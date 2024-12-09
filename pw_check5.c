#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ncurses.h>


#define MAX_USERS 100
#define MAX_OUTPUT_LEN 512


// Function to check password expiry information for a user
void check_password_expiry(const char *username, char *output) {
   char command[256];
   snprintf(command, sizeof(command), "sudo chage -l %s", username);


   FILE *fp = popen(command, "r");
   if (fp == NULL) {
       snprintf(output, MAX_OUTPUT_LEN, "Failed to get %s user's information.\n", username);
       return;
   }


   char buffer[256];
   time_t now = time(NULL);
   struct tm last_change_tm = {0};
   int last_change_found = 0;
   char last_change_date[256] = "N/A";


   while (fgets(buffer, sizeof(buffer), fp) != NULL) {
       if (strstr(buffer, "Last password change") != NULL) {
           strncpy(last_change_date, strchr(buffer, ':') + 2, sizeof(last_change_date));
           last_change_date[strlen(last_change_date) - 1] = '\0';  // Remove newline
           if (strptime(last_change_date, "%b %d, %Y", &last_change_tm) != NULL) {
               last_change_found = 1;
           }
       }
   }


   if (last_change_found) {
       time_t last_change_time = mktime(&last_change_tm);
       double days_diff = difftime(now, last_change_time) / (60 * 60 * 24);
       snprintf(output, MAX_OUTPUT_LEN, "User: %s, Last change: %s (%d days ago)\n",
                username, last_change_date, (int)days_diff);
   } else {
       snprintf(output, MAX_OUTPUT_LEN, "User: %s, Last change: %s\n", username, "No information");
   }


   pclose(fp);
}


// Function to get all users with shell access from /etc/passwd
int get_all_users(char users[MAX_USERS][256], int *total_users) {
   FILE *fp = fopen("/etc/passwd", "r");
   if (fp == NULL) {
       perror("/etc/passwd file cannot be opened");
       return -1;
   }


   char line[256];
   int user_count = 0;
   while (fgets(line, sizeof(line), fp) != NULL && user_count < MAX_USERS) {
       char temp_line[256];
       strncpy(temp_line, line, sizeof(temp_line));
       temp_line[sizeof(temp_line) - 1] = '\0';
       char *username = strtok(temp_line, ":");
       if (username == NULL) {
           continue;
       }


       char *last_colon = strrchr(line, ':');
       if (last_colon && (strstr(last_colon, "bash") || strstr(last_colon, "sh") || strstr(last_colon, "zsh"))) {
           strncpy(users[user_count], username, 256);
           users[user_count][255] = '\0';
           user_count++;
       }
   }


   fclose(fp);
   *total_users = user_count;
   return 0;
}


// Function to display user information with ncurses and pagination
void display_users_with_ncurses(const char users[MAX_USERS][256], int total_users) {
   int start_index = 0;
   char output[MAX_OUTPUT_LEN];


   initscr();              // Initialize ncurses
   cbreak();               // Immediate input processing
   noecho();               // Do not echo user input
   keypad(stdscr, TRUE);   // Enable arrow keys


   while (1) {
       clear(); // Clear screen


       // Get terminal size
       int rows, cols;
       (void)cols;
       getmaxyx(stdscr, rows, cols);


       int lines_per_page = rows - 4; // Exclude header/footer rows
       int end_index = start_index + lines_per_page;
       if (end_index > total_users) {
           end_index = total_users;
       }


       mvprintw(0, 0, "Displaying users %d to %d (Total: %d)", start_index + 1, end_index, total_users);
       mvprintw(1, 0, "[Enter/Space: Next page, q: Quit]\n");


       // Display user password information
       for (int i = start_index; i < end_index; i++) {
           check_password_expiry(users[i], output);
           mvprintw(3 + (i - start_index), 0, "%s", output);
       }


       refresh();


       // Handle user input
       int ch = getch();
       if (ch == 'q') {
           break; // Quit program
       } else if (ch == ' ' || ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
           if (end_index < total_users) {
               start_index += lines_per_page; // Move to the next page
           } else {
               mvprintw(rows - 1, 0, "End of list. Press 'q' to quit.");
               refresh();
               getch(); // Wait for input
               break;
           }
       }
   }


   endwin(); // End ncurses mode
}


int main() {
   char users[MAX_USERS][256];
   int total_users = 0;


   // Get all user information
   if (get_all_users(users, &total_users) == 0) {
       // Display user information with ncurses
       display_users_with_ncurses(users, total_users);
   } else {
       printf("Failed to get user information.\n");
   }


   return 0;
}







