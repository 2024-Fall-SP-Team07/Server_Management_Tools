#include "cpu_info_from_log.h"
#include "mem_info_from_log.h"
#include "sys_info.h"
#include "disk_info.h"
#include "network_info.h"
#include "resources_display.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <termio.h>
#include <curses.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>

const Unit_Mapping unitMap[] = {
    { KB, "KB" },
    { MB, "MB" },
    { GB, "GB" },
    { TB, "TB" },
    { PB, "PB" },
    { EB, "EB" }
};

struct sigaction sa;
UNIT MEM_unit = GB, SWAP_unit = MB, DISK_unit = GB;
int refresh_cycle = 1, cpu_usage_avg_duration = 3600, cpu_temp_avg_duration = 3600;
short jump_cnt = 0, return_exit = 0, blind = 0, display_cnt = 0, display_cnt_const = 0, list_count = 0, before_sec = -1, same_sec_cnt = 0;

void signal_handling(int sig){
    (void)sig;
    struct winsize wbuf;
    DateInfo date = get_Date();
    (ioctl(0, TIOCGWINSZ, &wbuf) == -1) ? fprintf(stderr, "%s\n", exception(-4, "main", "Windows Size", &date)) : 0;
    int input = getch();
    if (input != ERR){
        ('A' <= input && input <= 'Z') ? input += 32 : 0;
        switch(input){
            case 'd': 
                restore_screen_init();
                return_exit = 0;
                display_Disk_Info();
                break;
            case 's': 
                change_settings();
                display_clear(&wbuf, 0);
                break;
            case 'q': 
                restore_screen_init();
                return_exit = 1;
                break;
            case 'n':
                (jump_cnt > 0) ? display_cnt-- : 0;
                (jump_cnt > 0) ? jump_cnt-- : 0;
                break;
            case 'm':
                (jump_cnt < list_count - display_cnt_const) ? jump_cnt++ : 0;
                (display_cnt < list_count) ? display_cnt++ : 0;
                break;
            default:
                while ((input = getch()) != ERR);
        }
    }
}

void restore_screen_init(void){
    jump_cnt = 0;
    blind = 0;
    display_cnt = 0;
    display_cnt_const = 0;
    list_count = 0;
}

void initialization(void){
    struct winsize wbuf;
    DateInfo date = get_Date();
    int fd = fileno(stdin), now_row, now_col;
    char buf[LEN] = { '\0' };
    (ioctl(0, TIOCGWINSZ, &wbuf) == -1) ? fprintf(stderr, "%s\n", exception(-4, "main", "Windows Size", &date)) : 0;
    initscr();
    start_color();
    use_default_colors();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_MAGENTA, -1);
    curs_set(0);
    keypad(stdscr, TRUE);
    if (wbuf.ws_row < 23 || wbuf.ws_col < 110) {
        now_col = wbuf.ws_col;
        attron(COLOR_PAIR(1));
        sprintf(buf, "  !!! CAUTION !!!  ");
        move((now_row = wbuf.ws_row / 2 - 7), (now_col - strlen(buf)) / 2);
        addstr(buf);    
        attroff(COLOR_PAIR(1));
        sprintf(buf, "- The minimun terminal window size for running this program is ");
        move(now_row += 2, now_col = ((now_col - strlen(buf) - strlen("110 (col) x 23 (row).")) / 2));
        addstr(buf);
        attron(COLOR_PAIR(2) | A_BOLD);
        sprintf(buf, "110 (col) x 23 (row).");
        addstr(buf);
        attroff(COLOR_PAIR(2) | A_BOLD);
        sprintf(buf, "- Present terminal size is ");
        move(now_row += 1, now_col);
        addstr(buf);
        attron(COLOR_PAIR(2) | A_BOLD);
        sprintf(buf, "%d (col) x %d (row)", wbuf.ws_col, wbuf.ws_row);
        addstr(buf);
        attroff(COLOR_PAIR(2) | A_BOLD);
        sprintf(buf, "- So, Some information cannot be displayed, or UI can be broken.");
        move(now_row += 1, now_col);
        addstr(buf);
        move(now_row += 1, now_col);
        sprintf(buf, "- To see full information, Adjust the windows size.");
        addstr(buf);
        move(now_row += 2, now_col);
        sprintf(buf, "Press Any key to continue...");
        addstr(buf);
        getch();
    }

    fcntl(fd, F_SETOWN, getpid());
    fcntl(fd, F_SETFL, O_ASYNC | O_NONBLOCK);

    sa.sa_handler = signal_handling;
    sa.sa_flags = SA_NODEFER;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGIO, &sa, NULL);
    same_sec_cnt = 0;
}

void display_main(void){ 
    struct winsize wbuf;
    DateInfo date = get_Date();
    (ioctl(0, TIOCGWINSZ, &wbuf) == -1) ? fprintf(stderr, "%s\n", exception(-4, "main", "Windows Size", &date)) : 0;
    int now_row = 0, now_col = 0, fd, flags, status = 0;
    short line = 1;
    char buf[LEN] = { '\0' };
    NET_Result* net_info = NULL;
    display_clear(&wbuf, 0);
    while(1){
        if (return_exit == 1){
            display_clear(&wbuf, 0);
            return_exit = 0;
            fd = fileno(stdin);
            flags = fcntl(fd, F_GETFL, 0);
            fcntl(fd, F_SETOWN, 0);
            fcntl(fd, F_SETFL, flags & ~(O_ASYNC | O_NONBLOCK));
            endwin();
            return;
        }
        sigemptyset(&sa.sa_mask);
        move(1, (now_col = wbuf.ws_col * 0.01));
        sprintf(buf,"[Server Resources Monitor] (Refresh Cycle: %d seconds)", refresh_cycle);
        addstr(buf); 
        now_row = 3;
        now_col = wbuf.ws_col * 0.01;
        line = 12;
        move(now_row, now_col);
        display_System_Info(now_row, now_col);
        move(now_row += 1, now_col);

        display_CPU_Info(&wbuf, &status, cpu_temp_avg_duration, cpu_usage_avg_duration, now_row, now_col);
        if (status == 1){
            display_MEM_Info(&wbuf, now_row, now_col, MEM_unit, SWAP_unit);
        }
        net_info = display_NET_Info(&wbuf, net_info, &line, now_row, now_col);  

        attron(COLOR_PAIR(1));
        move(wbuf.ws_row - 3, 0);
        hline(' ' , wbuf.ws_col);
        move(wbuf.ws_row - 3, now_col);
        addstr("To see partition information, Press \"d\"");
        move(wbuf.ws_row - 2, 0);
        hline(' ' , wbuf.ws_col);
        move(wbuf.ws_row - 2, now_col);
        addstr("To change Viewer Settings, Press \"s\"");
        move(wbuf.ws_row - 1, 0);
        hline(' ' , wbuf.ws_col);
        move(wbuf.ws_row - 1, now_col);
        addstr("To restore main screen, Press \"q\"");
        attroff(COLOR_PAIR(1));
        move(now_row, now_col);
        refresh();
        sleep(1);
    }
    endwin();
}

void display_clear(struct winsize* wbuf, int repeat){
    for (int i = repeat; i < wbuf->ws_row; i++){
        move(i, 0);
        hline(' ', wbuf->ws_col);
    }
    refresh();
}

void display_System_Info(int now_row, int now_col){
    char buf[LEN] = { '\0' }, funcBuf[FUNC_BUF_LEN] = { '\0' };
    get_Hostname(funcBuf, sizeof(funcBuf));
    sprintf(buf, "Server: %s\t\t\t\t", funcBuf);
    addstr(buf);
    get_ProductName(funcBuf, sizeof(funcBuf));
    sprintf(buf, "Unit Model Name: %s", funcBuf);
    addstr(buf);
    move(now_row + 1, now_col);
    // Print System Time, Uptime
    get_SystemTime(funcBuf);
    sprintf(buf, "System Time: %s\t\t",funcBuf);
    addstr(buf);
    get_UpTime(funcBuf);
    sprintf(buf, "Uptime: %s", funcBuf);
    addstr(buf);
}

short check_running_collector(void* result, char* type){
    DateInfo log_date;
    // char buf[100] = {'\0'};
    log_date = (strcmp(type, "CPU") == 0) ? ((CPU_Result*) result)->date : ((MEM_Info*) result)->date;
    if (log_date.sec != before_sec) {
        same_sec_cnt = 0;
        before_sec = log_date.sec;
        return 1;
    } else {
        if (same_sec_cnt > 3) {
            before_sec = log_date.sec;
            return 0;
        } else {
            same_sec_cnt++;
        }
    }
    return 0;
}

void display_CPU_Info(struct winsize* wbuf, int* status, int temp_duration, int usage_duration, int now_row, int now_col){
    CPU_Result cpu_info = get_CPU_Information(1, 1);
    CPU_Result avg_cpu_info = get_CPU_Information(temp_duration, usage_duration);
    char buf[LEN] = { '\0' };
    // Print CPU Temperatrue
    attron(COLOR_PAIR(1));
    move(now_row + 2, 0);
    hline(' ' , wbuf->ws_col);
    move(now_row + 2, now_col);
    addstr("Temperature");
    attroff(COLOR_PAIR(1));
    move(now_row + 3, now_col);
    hline(' ', wbuf->ws_col);
    *status = check_running_collector(&cpu_info, "CPU");
    if (*status == 1){       
        addstr("CPU:      [");
        for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*(cpu_info.temp / 90))) ? addstr("#") : addstr(" "));
        addstr("]");
        (cpu_info.temp >= CPU_TEMP_CRITICAL_POINT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0; // Color change if temp >= CPU TEMP CRITICAL POINT
        sprintf(buf, " %.1f Celsius", cpu_info.temp);
        addstr(buf);
        (cpu_info.temp >= CPU_TEMP_CRITICAL_POINT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
        move(now_row + 4, now_col);
        hline(' ', wbuf->ws_col);
        addstr("CPU(AVG): [");
        for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*(avg_cpu_info.temp / 90))) ? addstr("#") : addstr(" "));
        addstr("]");
        (avg_cpu_info.temp >= CPU_TEMP_CRITICAL_POINT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0; // Color change if avg. temp >= CPU AVG TEMP CRITICAL POINT
        sprintf(buf, " %.1f Celsius ", avg_cpu_info.temp);
        addstr(buf);
        (avg_cpu_info.temp >= CPU_TEMP_CRITICAL_POINT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
        move(now_row + 4, wbuf->ws_col * (BAR_RATIO + 0.25));
        sprintf(buf, "(Duration: %d sec.)", temp_duration);
        addstr(buf);
    } else {
        hline(' ', wbuf->ws_col);
        addstr("    - Resources collector program is not running.");
        move(now_row + 4, now_col);
        hline(' ', wbuf->ws_col);
        addstr("    - Please check whether the program is running.");
    }

    // Print CPU Usage (instantaneous; %)
    attron(COLOR_PAIR(1));
    move(now_row + 6, 0);
    hline(' ', wbuf->ws_col);
    move(now_row + 6, now_col);
    addstr("CPU / Memory Usage");
    attroff(COLOR_PAIR(1));
    move(now_row + 7, now_col);
    hline(' ', wbuf->ws_col);
    if (*status == 1){
        addstr("CPU:      [");
        for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*(cpu_info.usage / 100))) ? addstr("#") : addstr(" "));
        addstr("]");
        (cpu_info.usage >= CPU_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0; // Color change if usage >= CPU USAGE CRITICAL POINT
        sprintf(buf, " %5.1f%%", cpu_info.usage);
        addstr(buf);
        (cpu_info.usage >= CPU_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
        // Print CPU Usage (Average during entered duration; %)
        move(now_row + 8, now_col);
        hline(' ', wbuf->ws_col);
        sprintf(buf, "CPU(AVG): [");
        addstr(buf);
        for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*(avg_cpu_info.usage / 100))) ? addstr("#") : addstr(" "));
        addstr("]");
        (avg_cpu_info.usage >= CPU_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0; // Color change if avg. usage >= CPU AVG USAGE CRITICAL POINT
        sprintf(buf, " %5.1f%%", avg_cpu_info.usage);
        addstr(buf);
        (avg_cpu_info.usage >= CPU_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
        move(now_row + 8, wbuf->ws_col * (BAR_RATIO + 0.2));
        sprintf(buf, "(Duration: %d sec.)", usage_duration);
        addstr(buf);
    } else {
        hline(' ', wbuf->ws_col);
        move(now_row + 8, now_col);
        hline(' ', wbuf->ws_col);
        addstr("    - Resources collector program is not running.");
        move(now_row + 9, now_col);
        hline(' ', wbuf->ws_col);
        addstr("    - Please check whether the program is running.");
        move(now_row + 10, now_col);
        hline(' ', wbuf->ws_col);
    }
}

void display_MEM_Info(struct winsize* wbuf, int now_row, int now_col, UNIT MEM_unit, UNIT SWAP_unit){
    MEM_Info mem_info = get_Mem_Information(1);
    char buf[LEN] = { '\0' };
    // Print Physical Memeory Usage
    move(now_row + 9, now_col);
    hline(' ', wbuf->ws_col);
    addstr("MEM:      [");
    for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total))) ? addstr("#") : addstr(" "));
    addstr("]");
    ((((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100) >= MEM_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    // Change usage color if usage >= MEM USAGE CRITICAL POINT
    sprintf(buf, " %5.1lf%%", ((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100);
    addstr(buf);
    ((((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100) >= MEM_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    move(now_row + 9, wbuf->ws_col * (BAR_RATIO + 0.2));
    addstr("(");
    ((((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100) >= MEM_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    // Change capacity color if usage >= MEM USAGE CRITICAL POINT
    sprintf(buf, "%.2lf%s", convert_Size_Unit(mem_info.size.mem_total - mem_info.size.mem_free, MEM_unit), unitMap[(int)MEM_unit].str);
    addstr(buf);
    ((((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100) >= MEM_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " / %.2lf%s)", convert_Size_Unit(mem_info.size.mem_total, MEM_unit), unitMap[(int)MEM_unit].str);
    addstr(buf);

    // Print Swap Memroy Usage
    move(now_row + 10, now_col);
    hline(' ', wbuf->ws_col);
    addstr("SWAP:     [");
    for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total))) ? addstr("#") : addstr(" "));
    addstr("]");
    ((((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100) > SWAP_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    // Change usage color if usage >= SWAP USAGE CRITICAL POINT
    if (mem_info.size.swap_total != 0) {
        sprintf(buf, " %5.1lf%%", ((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100);
    } else {
        sprintf(buf, " Not Used");
    }
    addstr(buf);
    ((((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100) > SWAP_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    move(now_row + 10, wbuf->ws_col * (BAR_RATIO + 0.2));
    addstr("(");
    ((((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100) > SWAP_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    // Change capacity color if usage >= SWAP USAGE CRITICAL POINT
    sprintf(buf, "%.2lf%s", convert_Size_Unit(mem_info.size.swap_total - mem_info.size.swap_free, SWAP_unit), unitMap[(int)SWAP_unit].str);
    addstr(buf);
    ((((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100) > SWAP_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " / %.2lf%s)", convert_Size_Unit(mem_info.size.swap_total, SWAP_unit), unitMap[(int)SWAP_unit].str);
    addstr(buf);  
}

NET_Result* display_NET_Info(struct winsize* wbuf, NET_Result *net_info, short* line, int now_row, int now_col) {
    NET_Result *cur_pos = NULL;
    UNIT NET_UP_Unit = KB, NET_Down_Unit = KB;
    DateInfo date = get_Date();
    float upSpeed = 0, downSpeed = 0;
    short max_net_ifa_name_len = 0, i = 0;
    char buf[LEN] = { '\0' };
    net_info = get_IPv4(net_info, &max_net_ifa_name_len, &list_count);
    if (net_info == NULL){
        fprintf(stderr, "%s\n", exception(-4, "display_NET_Info", "ifa list", &date));
        return NULL;
    }
    max_net_ifa_name_len += 2;
    move(now_row + (*line), 0);
    attron(COLOR_PAIR(1));
    hline(' ', wbuf->ws_col);
    move(now_row + (*line)++, now_col);
    sprintf(buf, "Networks  (# of Using Interface: %d)", list_count);
    addstr(buf);
    move(now_row + (*line), 0);
    hline(' ', wbuf->ws_col);
    move(now_row + (*line), now_col);
    addstr("Interface");
    move(now_row + (*line)++, now_col + max_net_ifa_name_len);
    sprintf(buf, "%16s  %9s  %9s %12s %12s %11s %11s", "IPv4 Address", "Upload", "Download", "Error(RX)", "Error(TX)", "Dropped(RX)", "Dropped(TX)");
    addstr(buf);
    attroff(COLOR_PAIR(1));

    get_Net_Byte(net_info);
    cur_pos = net_info;
    if (display_cnt - 1> list_count) {
        display_cnt--;
    }
    for (i = CPU_MEM_LINE + 1; i < wbuf->ws_row - 4; i++){
        move(i, 0);
        hline(' ', wbuf->ws_col);
    }
    for (int j = 0; (j < jump_cnt) && (blind == 1) && (display_cnt <= list_count); j++) {
        cur_pos = cur_pos -> next;
        continue;
    }
    for (i = 0; (cur_pos != NULL) && (display_cnt <= list_count) ; cur_pos = ((cur_pos != NULL) ? cur_pos -> next : cur_pos)){
        NET_UP_Unit = KB, NET_Down_Unit = KB;
        move(now_row + (*line) + i, now_col);
        hline(' ', wbuf->ws_col);
        sprintf(buf, "%s", cur_pos->ifa_name);
        addstr(buf);
        move(now_row + (*line) + i, now_col + max_net_ifa_name_len);
        hline(' ', wbuf->ws_col);
        move(now_row + (*line) + (i++), now_col + max_net_ifa_name_len);
        upSpeed = (cur_pos->transmit_byte[1] - cur_pos->transmit_byte[0]) / (float) 1024;
        downSpeed = (cur_pos->receive_byte[1] - cur_pos->receive_byte[0]) / (float) 1024;
        while(1){
            if (upSpeed >= 1024.0) {
                upSpeed /= (float)1024;
                NET_UP_Unit++;
                continue;
            }
            break;
        }
        while(1) {
            if (downSpeed >= 1024.0) {
                downSpeed /= (float)1024;
                NET_Down_Unit++;
                continue;
            }
            break;
        }

        sprintf(buf, "%16s  %5.1f%s/s  %5.1f%s/s %12ld %12ld %11ld %11ld", cur_pos->ipv4_addr, upSpeed, unitMap[(int)NET_UP_Unit].str, downSpeed, unitMap[(int)NET_Down_Unit].str, cur_pos->receive_error, cur_pos->transmit_error, cur_pos->receive_drop, cur_pos->transmit_drop);
        addstr(buf);

        if ((CPU_MEM_LINE + i - jump_cnt >= wbuf->ws_row - 4) && (cur_pos->next != NULL)){ 
            i--;
            break;
        }
    }
    display_cnt_const = (display_cnt_const == 0) ? i : display_cnt_const;
    display_cnt = (display_cnt == 0) ? i : display_cnt; 
    if ((list_count - display_cnt_const > 0)){ 
        blind = 1;
        move(wbuf->ws_row - 4, 0);
        hline(' ', wbuf->ws_col);
        move(wbuf->ws_row - 4, now_col);
        if (display_cnt_const == 0) {
            sprintf(buf, "...(%d rows omitted: %d / %d) - To see the remaining, ", list_count - display_cnt_const, display_cnt, list_count);
            addstr(buf);
            attron(COLOR_PAIR(2) | A_BOLD);
            addstr("Increase the terminal size and restart.");
            attroff(COLOR_PAIR(2) | A_BOLD);
        } else {
            sprintf(buf, "...(%d rows omitted: %d / %d) - To see the remaining, Press 'm' (Next) and 'n'(Previous)", list_count - display_cnt_const, display_cnt, list_count);
            addstr(buf);
        }
        move(wbuf->ws_row - 4, now_col);
    }
    return net_info;
}

void display_Disk_Info(void){
    struct winsize wbuf;
    DISK_Result* head = NULL, *next = NULL;
    short path_max_len = 0, fileSysetm_max_len = 0;
    int now_row = 1, now_col = 0, i;
    char buf[LEN] = { '\0' };
    DateInfo date = get_Date();
    (ioctl(0, TIOCGWINSZ, &wbuf) == -1) ? fprintf(stderr, "%s\n", exception(-4, "main", "Windows Size", &date)) : 0;

    display_clear(&wbuf, 0);
    move(now_row, (now_col = wbuf.ws_col * 0.01));
    // Print title
    sprintf(buf,"[Server Resources Monitor] (Refresh Cycle: %d seconds)", refresh_cycle);
    addstr(buf); 
    attron(COLOR_PAIR(1));
    move(wbuf.ws_row - 2, 0);
    hline(' ', wbuf.ws_col);
    move(wbuf.ws_row - 2, now_col);
    addstr("To change Viewer Settings, Press \"s\"");
    move(wbuf.ws_row - 1, 0);
    hline(' ', wbuf.ws_col);
    move(wbuf.ws_row - 1, now_col);
    addstr("To restore main screen, Press \"q\"");
    attroff(COLOR_PAIR(1));
    while(1){
        if (return_exit == 1){
            display_clear(&wbuf, 0);
            return_exit = 0;
            return;
        }
        head = get_Partition_Info_List(&list_count);
        if (head == NULL){
            fprintf(stderr, "%s\n", exception(-4, "display_Disk_Info", "Partition List", &date));
            continue;
        }
        path_max_len = get_Path_Max_Length(head) + 3;
        fileSysetm_max_len = get_fileSystem_Max_Length(head) + 3;
        now_row = 0;
        now_col = wbuf.ws_col * 0.01;
        move(now_row += 3, now_col);
        display_System_Info(now_row, now_col);
        move(now_row += 3, now_col);
        attron(COLOR_PAIR(1));
        move(now_row, 0);
        hline(' ' , wbuf.ws_col);
        move(now_row, now_col);
        sprintf(buf, "Disk Usage (# of Partition: %d)", list_count);
        addstr(buf);
        move(now_row += 1, 0);
        hline(' ' , wbuf.ws_col);
        move(now_row, now_col);
        addstr("FileSystem");
        move(now_row, now_col += fileSysetm_max_len);
        addstr("Mount Path");
        move(now_row, now_col += (path_max_len + wbuf.ws_col * (DISK_BAR_RATIO / 2)));
        addstr("Usage");
        move(now_row++, now_col += (wbuf.ws_col * (DISK_BAR_RATIO / 2) + 12));
        addstr("(Used   /   Total)");
        attroff(COLOR_PAIR(1));
        for (i = PARTITION_LINE + 1; i < wbuf.ws_row - 3; i++){
            move(i, 0);
            hline(' ', wbuf.ws_col);
        }
        for (int j = 0; (j < jump_cnt) && (blind == 1) && (display_cnt <= list_count); j++) {
            next = head->next;
            free(head);
            head = next;
            continue;
        }
        for (i = 0; head != NULL; i++){
            now_col = wbuf.ws_col * 0.01;
            move(now_row + i, now_col);
            sprintf(buf, "%s", head->fileSystem);
            addstr(buf);
            move (now_row + i, now_col =+ fileSysetm_max_len);
            sprintf(buf, "%s", head->mount_path);
            addstr(buf);
            move (now_row + i, now_col += path_max_len);
            addstr("[");
            for (int i = 0; i < wbuf.ws_col * DISK_BAR_RATIO; (i++ < (wbuf.ws_col*DISK_BAR_RATIO*((head->size.total_space - head->size.free_size) / (double)head->size.total_space))) ? addstr("#") : addstr(" "));
            addstr("]");
            (((head->size.total_space - head->size.free_size) / (double)head->size.total_space) * 100 >= DISK_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
            // Change usage color if usage >= DISK USAGE CRITICAL POINT
            sprintf(buf, "%5.1lf%%  ", ((head->size.total_space - head->size.free_size) / (double)head->size.total_space) * 100);
            addstr(buf);
            (((head->size.total_space - head->size.free_size) / (double)head->size.total_space) * 100 >= DISK_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
            move(now_row + i, now_col += ((wbuf.ws_col * DISK_BAR_RATIO) + 10));
            addstr("(");
            (((head->size.total_space - head->size.free_size) / (double)head->size.total_space) * 100 >= DISK_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
            // Change capacity color if usage >= DISK USAGE CRITICAL POINT
            sprintf(buf, "%.2lf%s", convert_Size_Unit(head->size.total_space - head->size.free_size, DISK_unit), unitMap[(int)DISK_unit].str);
            addstr(buf);
            (((head->size.total_space - head->size.free_size) / (double)head->size.total_space) * 100 >= DISK_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
            sprintf(buf, " / %.2lf%s)", convert_Size_Unit(head->size.total_space, DISK_unit), unitMap[(int)DISK_unit].str);
            addstr(buf);

            if ((PARTITION_LINE + i - jump_cnt >= wbuf.ws_row - 4) && (head->next != NULL)){ 
                break;
            }

            next = head->next;
            free(head);
            head = next;
        }
        display_cnt_const = (display_cnt_const == 0) ? i : display_cnt_const;
        display_cnt = (display_cnt == 0) ? i : display_cnt; 
        if ((list_count - display_cnt_const > 0)){ 
            blind = 1;
            move(wbuf.ws_row - 3, 0);
            hline(' ', wbuf.ws_col);
            move(wbuf.ws_row - 3, now_col = wbuf.ws_col * 0.01);
            if (display_cnt_const == 0) {
                sprintf(buf, "...(%d rows omitted: %d / %d) - To see the remaining, ", list_count - display_cnt_const, display_cnt, list_count);
                addstr(buf);
                attron(COLOR_PAIR(2) | A_BOLD);
                addstr("Increase the terminal size and restart.");
                attroff(COLOR_PAIR(2) | A_BOLD);
            } else {
                sprintf(buf, "...(%d rows omitted: %d / %d) - To see the remaining, Press 'm' (Next) and 'n'(Previous)", list_count - display_cnt_const, display_cnt, list_count);
                addstr(buf);
            }
            move(wbuf.ws_row - 3, now_col);
        }
        refresh();   
        sleep(refresh_cycle);
    }
}

void change_settings(void){
    struct winsize wbuf;
    int now_row, now_col, ch, current_line = 1, interval_input, mem_unit_input, cpu_temp_avg_duration_input, cpu_usage_avg_duration_input;
    int swap_unit_input, disk_unit_input, inputCnt = 0, row, col;
    char inputBuf[3] = { '\0' };
    DateInfo date = get_Date();
    (ioctl(0, TIOCGWINSZ, &wbuf) == -1) ? fprintf(stderr, "%s\n", exception(-4, "main", "Windows Size", &date)) : 0;
    curs_set(1);
    display_clear(&wbuf, 0);
    inputCnt = 0;
    while(1){
        now_row = 1;
        now_col = wbuf.ws_col * 0.01;
        move(now_row, now_col);
        attron(COLOR_PAIR(1));
        move(now_row, 0);
        hline(' ', wbuf.ws_col);
        move(now_row, now_col);
        addstr("Viewer Setting");
        attroff(COLOR_PAIR(1));
        move(now_row + 1, now_col += 4);
        addstr("1. Refresh interval (seconds, Integer >= 1): ");
        move(now_row + 2, now_col);
        addstr("2. CPU Temperature Average Duration (seconds, Integer >= 1): ");
        move(now_row + 3, now_col);
        addstr("3. CPU Usage Average Duration (seconds, Integer >= 1): ");
        move(now_row + 5, now_col);
        addstr("4. Memory Space Unit: ");
        move(now_row + 6, now_col);
        addstr("5. SWAP Space Unit: ");
        move(now_row + 7, now_col);
        addstr("6. Disk (Partition) Space Unit: ");
        
        move(wbuf.ws_row - 12, now_col);
        attron(COLOR_PAIR(1));
        addch(' ');
        addch(' ');
        addstr("[Configuration Input Guide]  ");
        attroff(COLOR_PAIR(1));
        move(wbuf.ws_row - 10, now_col);
        addstr("- 1: Update interval for values read by the program, in seconds.");
        move(wbuf.ws_row - 9, now_col);
        addstr("- 2-3: Interval for calculating the avg. CPU Temp./CPU Usage, in seconds.");
        move(wbuf.ws_row - 8, now_col);
        addstr("  (e.g. 10 -> calculate the avg. temp./usage over the past 10 sec.)");
        move(wbuf.ws_row - 7, now_col);
        addstr("- 4-6: Unit for Memory/SWAP/Disk (Partition) Capacity. Enter the unit in English, case-insensitive.");
        move(wbuf.ws_row - 6, now_col);
        addstr("  (e.g. kb, MB, gB, ... - supported. / k, M, g, ... - not supported.)");
        move(wbuf.ws_row - 4, now_col);
        addstr("$$ Supported units: KB, MB, GB, TB, PB, EB $$");
        now_col -= 4;
        attron(COLOR_PAIR(1));
        move(wbuf.ws_row - 2, 0);
        hline(' ', wbuf.ws_col);
        move(wbuf.ws_row - 2, now_col);
        addstr("- Press Enter after entering each value to move of cursor to the next field.");
        move(wbuf.ws_row - 1, 0);
        hline(' ', wbuf.ws_col);
        move(wbuf.ws_row - 1, now_col); 
        addstr("- Once all field are filled, the program will return to the previous screen and entered value is applied.");
        attroff(COLOR_PAIR(1));
        move(now_row += 1, now_col = strlen("     1. Refresh interval (seconds, Integer >= 1): "));

        while ((ch = getch()) != ERR);
        while (1) {
            ch = getch();
            if (ch == ERR){
                continue;
            } else {
                if (ch == '\n' || ch == '\r') {
                    switch(current_line) {
                        case 1:
                            interval_input = atoi(inputBuf);
                            inputCnt = 0;
                            current_line++;
                            move(now_row += 1, now_col = strlen("     2. CPU Temperature Average Duration (seconds, Integer >= 1): ")) ;
                            break;
                        case 2:
                            cpu_temp_avg_duration_input = atoi(inputBuf);
                            inputCnt = 0;
                            current_line++;
                            move(now_row += 1, now_col = strlen("     3. CPU Usage Average Duration (seconds, Integer >= 1): "));
                            break;
                        case 3:
                            cpu_usage_avg_duration_input = atoi(inputBuf);
                            inputCnt = 0;
                            current_line++;
                            move(now_row += 2, now_col = strlen("     4. Memory Space Unit: "));
                            break;
                        case 4:
                            mem_unit_input = find_idx_unit(inputBuf);
                            inputCnt = 0;
                            current_line++;
                            move(now_row += 1, now_col = strlen("     5. SWAP Space Unit: "));
                            break;
                        case 5:
                            swap_unit_input = find_idx_unit(inputBuf);
                            inputCnt = 0;
                            current_line++;
                            move(now_row += 1, now_col = strlen("     6. Disk (Partition) Space Unit: "));
                            break;
                        default: 
                            disk_unit_input = find_idx_unit(inputBuf);
                            inputCnt = 0;
                            current_line = 0;
                            refresh_cycle = interval_input;
                            cpu_temp_avg_duration = cpu_temp_avg_duration_input;
                            cpu_usage_avg_duration = cpu_usage_avg_duration_input;
                            MEM_unit = mem_unit_input;
                            SWAP_unit = swap_unit_input;
                            DISK_unit = disk_unit_input;
                            curs_set(0);
                            return;
                    }
                } if (ch == KEY_BACKSPACE || ch == 127) { // Backspace
                    if (inputCnt > 0) {
                        inputBuf[inputCnt--] = '\0';
                        move(now_row, --now_col);
                        addch(' ');
                        move(now_row, now_col);
                    } else {
                        move(now_row, now_col);
                    }
                } else if (isalpha(ch) != 0 || isdigit(ch) != 0){
                    inputBuf[inputCnt++] = ch;
                    inputBuf[inputCnt] = '\0';
                    addch(ch);
                    getyx(stdscr, row, col);
                    move((now_row = row), col - 1);
                    addch(' ');
                    move((now_row = row), (now_col = col - 1));
                }
            }
        }
        refresh();
    }
    return;
}

int find_idx_unit(char* str){
    for (int i = 0; i < 2; i++){
        if ('a' <= str[i] && str[i] <= 'z'){
            str[i] = str[i] - 32;
        }
    }
    for (int i = 0; i < UNIT_COUNT; i++) {
        if (strcmp(unitMap[i].str, str) == 0) {
            return i;
        }
    }
    return -1;
}