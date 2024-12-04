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
int refresh_cycle = 1, cpu_usage_avg_duration = 3600, cpu_temp_avg_duration = 3600, return_exit = 0;

void signal_handling(int sig){
    struct winsize wbuf;
    (ioctl(0, TIOCGWINSZ, &wbuf) == -1) ? printf("%s", exception(-4, "main", "Windows Size")) : 0;
    int input = getch();
    char buf[LEN] =  {'\0' };
    if (input != ERR){
        switch(input){
            case 'd': 
                display_Disk_Info();
                break;
            case 's': 
                change_settings();
                display_clear(&wbuf, 0);
                break;
            case 'q': 
                return_exit = 1;
                break;
        }
    }
}

void initialization(void){
    struct winsize wbuf;
    
    int fd = fileno(stdin);
    fcntl(fd, F_SETOWN, getpid());
    fcntl(fd, F_SETFL, O_ASYNC | O_NONBLOCK);

    sa.sa_handler = signal_handling;
    sa.sa_flags = SA_NODEFER;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGIO, &sa, NULL);

    (ioctl(0, TIOCGWINSZ, &wbuf) == -1) ? printf("%s", exception(-4, "main", "Windows Size")) : 0;
    initscr();
    start_color();
    use_default_colors();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_MAGENTA, -1);
    curs_set(0);
    keypad(stdscr, TRUE);
}

void display_main(void){ // Print Server Hostname, IPv4 Address
    struct winsize wbuf;
    (ioctl(0, TIOCGWINSZ, &wbuf) == -1) ? printf("%s", exception(-4, "main", "Windows Size")) : 0;
    int now_row = 0, now_col = 0, input, fd, flags;
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
        // while ((input = getch()) != ERR);
        sigemptyset(&sa.sa_mask);
        move(1, (now_col = wbuf.ws_col * 0.01));
        sprintf(buf,"[Server Resources Monitor] (Refresh Cycle: %d seconds)", refresh_cycle);
        addstr(buf); 
        now_row = 3;
        now_col = wbuf.ws_col * 0.01;
        line = 2;
        move(now_row, now_col);
        display_System_Info(now_row, now_col);
        move(now_row += 1, now_col);
        display_CPU_Info(&wbuf, &line, cpu_temp_avg_duration, cpu_usage_avg_duration, now_row, now_col);
        display_MEM_Info(&wbuf, &line, now_row, now_col, MEM_unit, SWAP_unit);
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

void display_CPU_Info(struct winsize* wbuf, short* line, int temp_duration, int usage_duration, int now_row, int now_col){
    CPU_Result cpu_info = get_CPU_Information(1, 1);
    CPU_Result avg_cpu_info = get_CPU_Information(temp_duration, usage_duration);
    char buf[LEN] = { '\0' };

    // Print CPU Temperatrue
    attron(COLOR_PAIR(1));
    move(now_row + (*line), 0);
    hline(' ' , wbuf->ws_col);
    move(now_row + (*line)++, now_col);
    addstr("Temperature");
    attroff(COLOR_PAIR(1));
    move(now_row + (*line)++, now_col);
    hline(' ', wbuf->ws_col);
    addstr("CPU:      [");
    for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*(cpu_info.temp / 90))) ? addstr("#") : addstr(" "));
    addstr("]");
    (cpu_info.temp >= CPU_TEMP_CRITICAL_POINT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " %.1f Celsius", cpu_info.temp);
    addstr(buf);
    (cpu_info.temp >= CPU_TEMP_CRITICAL_POINT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    move(now_row + (*line), now_col);
    hline(' ', wbuf->ws_col);
    addstr("CPU(AVG): [");
    for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*(avg_cpu_info.temp / 90))) ? addstr("#") : addstr(" "));
    addstr("]");
    (avg_cpu_info.temp >= CPU_TEMP_CRITICAL_POINT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " %.1f Celsius ", avg_cpu_info.temp);
    addstr(buf);
    (avg_cpu_info.temp >= CPU_TEMP_CRITICAL_POINT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    move(now_row + (*line)++, wbuf->ws_col * (BAR_RATIO + 0.25));
    sprintf(buf, "(Duration: %d sec.)", temp_duration);
    addstr(buf);
    (*line)++;

    // Print CPU Usage (instantaneous; %)
    attron(COLOR_PAIR(1));
    move(now_row + (*line), 0);
    hline(' ', wbuf->ws_col);
    move(now_row + (*line)++, now_col);
    addstr("CPU / Memory Usage");
    attroff(COLOR_PAIR(1));
    move(now_row + (*line)++, now_col);
    hline(' ', wbuf->ws_col);
    addstr("CPU:      [");
    for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*(cpu_info.usage / 100))) ? addstr("#") : addstr(" "));
    addstr("]");
    (cpu_info.usage >= CPU_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " %5.1f%%", cpu_info.usage);
    addstr(buf);
    (cpu_info.usage >= CPU_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;

    // Print CPU Usage (Average during entered duration; %)
    move(now_row + (*line), now_col);
    hline(' ', wbuf->ws_col);
    sprintf(buf, "CPU(AVG): [");
    addstr(buf);
    for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*(avg_cpu_info.usage / 100))) ? addstr("#") : addstr(" "));
    addstr("]");
    (avg_cpu_info.usage >= CPU_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " %5.1f%%", avg_cpu_info.usage);
    addstr(buf);
    (avg_cpu_info.usage >= CPU_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    move(now_row + (*line)++, wbuf->ws_col * (BAR_RATIO + 0.2));
    sprintf(buf, "(Duration: %d sec.)", usage_duration);
    addstr(buf);
}

void display_MEM_Info(struct winsize* wbuf, short* line, int now_row, int now_col, UNIT MEM_unit, UNIT SWAP_unit){
    MEM_Info mem_info = get_Mem_Information(1);
    char buf[LEN] = { '\0' };
    // Print Physical Memeory Usage
    move(now_row + (*line), now_col);
    hline(' ', wbuf->ws_col);
    addstr("MEM:      [");
    for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total))) ? addstr("#") : addstr(" "));
    addstr("]");
    ((((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100) >= MEM_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " %5.1lf%%", ((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100);
    addstr(buf);
    ((((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100) >= MEM_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    move(now_row + (*line)++, wbuf->ws_col * (BAR_RATIO + 0.2));
    addstr("(");
    ((((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100) >= MEM_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, "%.2lf%s", convert_Size_Unit(mem_info.size.mem_total - mem_info.size.mem_free, MEM_unit), unitMap[(int)MEM_unit].str);
    addstr(buf);
    ((((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100) >= MEM_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " / %.2lf%s)", convert_Size_Unit(mem_info.size.mem_total, MEM_unit), unitMap[(int)MEM_unit].str);
    addstr(buf);

    // Print Swap Memroy Usage
    move(now_row + (*line), now_col);
    hline(' ', wbuf->ws_col);
    addstr("SWAP:     [");
    for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total))) ? addstr("#") : addstr(" "));
    addstr("]");
    ((((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100) > SWAP_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    if (mem_info.size.swap_total != 0) {
        sprintf(buf, " %5.1lf%%", ((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100);
    } else {
        sprintf(buf, " Not Used");
    }
    addstr(buf);
    ((((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100) > SWAP_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    move(now_row + (*line)++, wbuf->ws_col * (BAR_RATIO + 0.2));
    addstr("(");
    ((((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100) > SWAP_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, "%.2lf%s", convert_Size_Unit(mem_info.size.swap_total - mem_info.size.swap_free, SWAP_unit), unitMap[(int)SWAP_unit].str);
    addstr(buf);
    ((((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100) > SWAP_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " / %.2lf%s)", convert_Size_Unit(mem_info.size.swap_total, SWAP_unit), unitMap[(int)SWAP_unit].str);
    addstr(buf);
    (*line)++;       
}

NET_Result* display_NET_Info(struct winsize* wbuf, NET_Result *net_info, short* line, int now_row, int now_col) {
    NET_Result *cur_pos = NULL;
    UNIT NET_UP_Unit = KB, NET_Down_Unit = KB;
    float upSpeed = 0, downSpeed = 0;
    short max_net_ifa_name_len = 0, ifa_count = 0;
    char buf[LEN] = { '\0' };
    int col, row;
    net_info = get_IPv4(net_info, &max_net_ifa_name_len, &ifa_count);
    max_net_ifa_name_len += 2;
    move(now_row + (*line), 0);
    attron(COLOR_PAIR(1));
    hline(' ', wbuf->ws_col);
    move(now_row + (*line)++, now_col);
    sprintf(buf, "Networks  (# of Using Interface: %d)", ifa_count);
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
    for (int i = 0; cur_pos != NULL ; cur_pos = cur_pos -> next){
        getyx(stdscr, row, col);
        (void)col;
        if (wbuf->ws_row - row - 3 == 0 ){
            move(now_row + (*line) + i - 1, now_col);
            sprintf(buf, "...(%d rows omitted) - To see the remaining, Increase the terminal size.", ifa_count - (i++) + 1);
            addstr(buf);
            move(now_row + (*line) + i - 1, now_col);
            break;
        }
        NET_UP_Unit = KB, NET_Down_Unit = KB;
        move(now_row + (*line) + i, now_col);
        hline(' ', wbuf->ws_col);
        sprintf(buf, "%s", cur_pos->ifa_name);
        addstr(buf);

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
    }
    return net_info;
}

void display_Disk_Info(){
    struct winsize wbuf;
    DISK_Result* head = NULL, *next = NULL;
    short path_max_len = 0, fileSysetm_max_len = 0, partition_count = 0;
    int now_row = 1, now_col = 0, input, row, col;
    char buf[LEN] = { '\0' };
    (ioctl(0, TIOCGWINSZ, &wbuf) == -1) ? printf("%s", exception(-4, "main", "Windows Size")) : 0;

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
        head = get_Partition_Info_List(&partition_count);
        path_max_len = get_Path_Max_Length(head) + 3;
        fileSysetm_max_len = get_fileSystem_Max_Length(head) + 3;
        now_row = 0;
        now_col = now_col = wbuf.ws_col * 0.01;
        move(now_row += 3, now_col);
        display_System_Info(now_row, now_col);
        move(now_row += 3, now_col);
        attron(COLOR_PAIR(1));
        move(now_row, 0);
        hline(' ' , wbuf.ws_col);
        move(now_row, now_col);
        sprintf(buf, "Disk Usage (# of Partition: %d)", partition_count);
        addstr(buf);
        move(now_row += 1, 0);
        hline(' ' , wbuf.ws_col);
        move(now_row, now_col);
        addstr("FileSystem");
        move(now_row, now_col += fileSysetm_max_len);
        addstr("Mount Path");
        move(now_row, now_col += (path_max_len + wbuf.ws_col * (DISK_BAR_RATIO / 2)));
        addstr("Usage");
        move(now_row, now_col += (wbuf.ws_col * (DISK_BAR_RATIO / 2) + 12));
        addstr("(Used   /   Total)");
        attroff(COLOR_PAIR(1));
        for (int i = 1; head != NULL; i++){
            now_col = wbuf.ws_col * 0.01;
            getyx(stdscr, row, col);
            (void)col;
            if (wbuf.ws_row - row - 3 == 0){
                move(now_row + i - 1, now_col);
                hline(' ', wbuf.ws_col);
                sprintf(buf, "...(%d rows omitted) - To see the remaining, Increase the terminal size.", partition_count - (i++) + 2);
                addstr(buf);
                move(now_row + i - 1, now_col);
                break;
            }
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
            sprintf(buf, "%5.1lf%%  ", ((head->size.total_space - head->size.free_size) / (double)head->size.total_space) * 100);
            addstr(buf);
            (((head->size.total_space - head->size.free_size) / (double)head->size.total_space) * 100 >= DISK_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
            move(now_row + i, now_col += ((wbuf.ws_col * DISK_BAR_RATIO) + 10));
            addstr("(");
            (((head->size.total_space - head->size.free_size) / (double)head->size.total_space) * 100 >= DISK_USAGE_CRITICAL_PERCENT) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
            sprintf(buf, "%.2lf%s", convert_Size_Unit(head->size.total_space - head->size.free_size, DISK_unit), unitMap[(int)DISK_unit].str);
            addstr(buf);
            (((head->size.total_space - head->size.free_size) / (double)head->size.total_space) * 100 >= DISK_USAGE_CRITICAL_PERCENT) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
            sprintf(buf, " / %.2lf%s)", convert_Size_Unit(head->size.total_space, DISK_unit), unitMap[(int)DISK_unit].str);
            addstr(buf);
            next = head->next;
            free(head);
            head = next;
        }
        refresh();   
        sleep(refresh_cycle);
    }
}

void change_settings(){
    struct winsize wbuf;
    int now_row, now_col, ch, current_line = 1, interval_input, mem_unit_input, cpu_temp_avg_duration_input, cpu_usage_avg_duration_input;
    int swap_unit_input, disk_unit_input, inputCnt = 0, row, col;
    char inputBuf[3] = { '\0' }, buf[LEN] = { '\0' };
    (ioctl(0, TIOCGWINSZ, &wbuf) == -1) ? printf("%s", exception(-4, "main", "Windows Size")) : 0;
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
        addstr("1. Refresh interval (seconds, Integer): ");
        move(now_row + 3, now_col);
        addstr("2. CPU Temperature Average Duration (seconds, Integer): ");
        move(now_row + 5, now_col);
        addstr("3. CPU Usage Average Duration (seconds, Integer): ");
        move(now_row + 7, now_col);
        addstr("4. Memory Space Unit: ");
        move(now_row + 9, now_col);
        addstr("5. SWAP Space Unit: ");
        move(now_row + 11, now_col);
        addstr("6. Disk (Partition) Space Unit: ");
        
        move(wbuf.ws_row - 15, now_col);
        attron(COLOR_PAIR(1));
        addch(' ');
        addch(' ');
        addstr("[Configuration Input Guide]  ");
        attroff(COLOR_PAIR(1));
        move(wbuf.ws_row - 13, now_col);
        addstr("- 1: Update interval for values read by the program, in seconds. (value must be integer >= 1)");
        move(wbuf.ws_row - 12, now_col);
        addstr("  (e.g. 1 -> updates the value every second.)");
        move(wbuf.ws_row - 10, now_col);
        addstr("- 2-3: Period for calculating the avg. CPU Temp./CPU Usage, in seconds. (value must be integer >= 1)");
        move(wbuf.ws_row - 9, now_col);
        addstr("  (e.g. 10 -> calculate the avg. temp./usage over the past 10 sec.)");
        move(wbuf.ws_row - 7, now_col);
        addstr("- 4-6: Unit for Memory/SWAP/Disk (Partition) Capacity. Enter the unit in English, case-insensitive.");
        move(wbuf.ws_row - 6, now_col);
        addstr("  (e.g. kb, MB, gB, ... - supported. / k, M, g, ... - not supported.)");
        move(wbuf.ws_row - 4, now_col);
        addstr("$ Supported units: KB, MB, GB, TB, PB, EB");
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
        move(now_row += 1, now_col = strlen("     1. Refresh interval (seconds, Integer): "));

        /*
        설정 값 입력 안내
        - 1: 프로그램이 읽은 값들이 갱신되는 주기. 단위는 "초" 이며, 1 이상의 정수를 입력해야 함. (e.g. 1 -> 1초 간격으로 값을 갱신)
        - 2-3: CPU 온도/CPU 점유율의 평균값을 구할 기간. 단위는 "초" 이며, 1 이상의 정수를 입력해야 함.(e.g. 10 -> 갱신 시점으로부터 10초동안의 온도 평균 계산)
        - 4-6: 메모리 / SWAP / 디스크 파티션 용량의 단위를 설정. 설정할 단위를 대소문자 구분 없이 영어로 입력. (e.g. kb, MB, gB, ...)
        ※ 지원 가능한 용량 단위: KB, MB, GB, TB, PB, EB

        
        */
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
                            move(now_row += 2, now_col = strlen("     2. CPU Temperature Average Duration (seconds, Integer): ")) ;
                            break;
                        case 2:
                            cpu_temp_avg_duration_input = atoi(inputBuf);
                            inputCnt = 0;
                            current_line++;
                            move(now_row += 2, now_col = strlen("     3. CPU Usage Average Duration (seconds, Integer): "));
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
                            move(now_row += 2, now_col = strlen("     5. SWAP Space Unit: "));
                            break;
                        case 5:
                            swap_unit_input = find_idx_unit(inputBuf);
                            inputCnt = 0;
                            current_line++;
                            move(now_row += 2, now_col = strlen("     6. Disk (Partition) Space Unit: "));
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