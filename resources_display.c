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

const Unit_Mapping unitMap[] = {
    { KB, "KB" },
    { MB, "MB" },
    { GB, "GB" },
    { TB, "TB" },
    { PB, "PB" },
    { EB, "EB" }
};

void display_main(void){ // Print Server Hostname, IPv4 Address
    struct winsize wbuf;
    int now_row = 0, now_col = 0, duration = 10, refresh_cycle = 1;
    short line = 1;
    char buf[LEN] = { '\0' };
    UNIT MEM_unit = GB, SWAP_unit = MB, DISK_unit = GB;
    NET_Result* net_info = NULL;

    (ioctl(0, TIOCGWINSZ, &wbuf) == -1) ? printf("%s", exception(-4, "main", "Windows Size")) : 0;
    initscr();
    start_color();
    use_default_colors();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_MAGENTA, -1);
    curs_set(0);
    move(line++, (now_col = wbuf.ws_col * 0.01));
    // Print title
    sprintf(buf,"[Server Resources Monitor] (Refresh Cycle: %d seconds)", refresh_cycle);
    addstr(buf); 
    
    while(1){
        line = 1;
        line++;
        display_System_Info(&wbuf, &now_row, &now_col);
        display_CPU_Info(&wbuf, &line, duration, now_row, now_col);
        display_MEM_Info(&wbuf, &line, now_row, now_col, MEM_unit, SWAP_unit);
        net_info = display_NET_Info(&wbuf, net_info, &line, now_row, now_col);  

        attron(COLOR_PAIR(1));
        move(wbuf.ws_row - 2, 0);
        hline(' ' , wbuf.ws_col);
        move(wbuf.ws_row - 2, now_col);
        sprintf(buf, "To see partition information, Press \"d\"");
        addstr(buf);
        move(wbuf.ws_row - 1, 0);
        hline(' ' , wbuf.ws_col);
        move(wbuf.ws_row - 1, now_col);
        addstr("To change Viewer Settings, Press \"s\"");
        attroff(COLOR_PAIR(1));
        move(now_row, now_col);
        refresh();
        sleep(refresh_cycle);
    }
    getch();
    endwin();
}

void display_System_Info(struct winsize* wbuf, int* now_row, int* now_col){
    char buf[LEN] = { '\0' }, funcBuf[FUNC_BUF_LEN] = { '\0' };
    move(((*now_row) = wbuf->ws_row * 0.1), ((*now_col) = wbuf->ws_col * 0.01));
    get_Hostname(funcBuf, sizeof(funcBuf));
    sprintf(buf, "Server: %s\t\t\t\t", funcBuf);
    addstr(buf);
   
    get_ProductName(funcBuf, sizeof(funcBuf));
    sprintf(buf, "Unit Model Name: %s\t\t", funcBuf);
    addstr(buf);
    move((*now_row) += 1, *now_col);
    // Print System Time, Uptime
    get_SystemTime(funcBuf);
    sprintf(buf, "System Time: %s\t\t",funcBuf);
    addstr(buf);
    get_UpTime(funcBuf);
    sprintf(buf, "Uptime: %s", funcBuf);
    addstr(buf);
}

void display_CPU_Info(struct winsize* wbuf, short* line, int duration, int now_row, int now_col){
    CPU_Result cpu_info = get_CPU_Information(1);
    CPU_Result avg_cpu_info = get_CPU_Information(duration);
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
    (cpu_info.temp >= 70) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " %.1f Celsius", cpu_info.temp);
    addstr(buf);
    (cpu_info.temp >= 70) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    move(now_row + (*line), now_col);
    hline(' ', wbuf->ws_col);
    addstr("CPU(AVG): [");
    for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*(avg_cpu_info.temp / 90))) ? addstr("#") : addstr(" "));
    addstr("]");
    (avg_cpu_info.temp >= 70) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " %.1f Celsius ", avg_cpu_info.temp);
    addstr(buf);
    (avg_cpu_info.temp >= 70) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    move(now_row + (*line)++, wbuf->ws_col * (BAR_RATIO + 0.25));
    sprintf(buf, "(Duration: %d sec.)", duration);
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
    (cpu_info.usage >= 85) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " %5.1f%%", cpu_info.usage);
    addstr(buf);
    (cpu_info.usage >= 85) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;

    // Print CPU Usage (Average during entered duration; %)
    move(now_row + (*line), now_col);
    hline(' ', wbuf->ws_col);
    sprintf(buf, "CPU(AVG): [");
    addstr(buf);
    for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*(avg_cpu_info.usage / 100))) ? addstr("#") : addstr(" "));
    addstr("]");
    (avg_cpu_info.usage >= 85) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " %5.1f%%", avg_cpu_info.usage);
    addstr(buf);
    (avg_cpu_info.usage >= 85) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    move(now_row + (*line)++, wbuf->ws_col * (BAR_RATIO + 0.2));
    sprintf(buf, "(Duration: %d sec.)", duration);
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
    addstr(")");
    ((((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100) >= 85) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " %5.1lf%%", ((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100);
    addstr(buf);
    ((((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100) >= 85) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    move(now_row + (*line)++, wbuf->ws_col * (BAR_RATIO + 0.2));
    addstr("(");
    ((((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100) >= 85) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, "%.2lf%s", convert_Size_Unit(mem_info.size.mem_total - mem_info.size.mem_free, MEM_unit), unitMap[(int)MEM_unit].str);
    addstr(buf);
    ((((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100) >= 85) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " / %.2lf%s)", convert_Size_Unit(mem_info.size.mem_total, MEM_unit), unitMap[(int)MEM_unit].str);
    addstr(buf);

    // Print Swap Memroy Usage
    move(now_row + (*line), now_col);
    hline(' ', wbuf->ws_col);
    addstr("SWAP:     [");
    for (int i = 0; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total))) ? addstr("#") : addstr(" "));
    addstr("]");
    ((((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100) > 0) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, " %5.1lf%%", ((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100);
    addstr(buf);
    ((((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100) > 0) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
    move(now_row + (*line)++, wbuf->ws_col * (BAR_RATIO + 0.2));
    addstr("(");
    ((((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100) > 0) ? attron(COLOR_PAIR(2) | A_BOLD) : 0;
    sprintf(buf, "%.2lf%s", convert_Size_Unit(mem_info.size.swap_total - mem_info.size.swap_free, SWAP_unit), unitMap[(int)SWAP_unit].str);
    addstr(buf);
    ((((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100) > 0) ? attroff(COLOR_PAIR(2) | A_BOLD) : 0;
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
        if (row == wbuf->ws_row - 6){
            move(now_row + (*line) + i + 1, now_col);
            sprintf(buf, "...(%d rows omitted) - To see the remaining, Increase the terminal size.", ifa_count - (i++));
            addstr(buf);
            move(now_row + (*line) + i + 1, now_col);
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

void display_Disk_Info(struct winsize* wbuf, int now_row, int now_col, UNIT DISK_unit){
    DISK_Result* head = NULL, *next = NULL;
    int path_max_len = 0;
    char buf[LEN] = { '\0' };
    move (now_row + 12, now_col);
    addstr("Disk Usage");
    head = get_Partition_Info_List();
    path_max_len = get_Partition_Max_Length(head) + 1;
    for (int i = 13; head != NULL; i++){
        move (now_row + i, now_col);
        sprintf(buf, "%s (%s): ", head->fileSystem, head->mount_path);
        addstr(buf);
        move (now_row + i, now_col + path_max_len + 1);
        addstr(" [");
        for (int i = path_max_len * 0.1; i < wbuf->ws_col * BAR_RATIO; (i++ < (wbuf->ws_col*BAR_RATIO*((head->size.total_space - head->size.free_size) / (double)head->size.total_space))) ? addstr("#") : addstr(" "));
        sprintf(buf, "] %5.1lf%%", ((head->size.total_space - head->size.free_size) / (double)head->size.total_space) * 100);
        addstr(buf);
        move(now_row  + i, wbuf->ws_col * (BAR_RATIO + 0.2));
        sprintf(buf, "(%.2lf%s / %.2lf%s)", convert_Size_Unit(head->size.total_space - head->size.free_size, DISK_unit), unitMap[(int)DISK_unit].str, convert_Size_Unit(head->size.total_space, DISK_unit), unitMap[(int)DISK_unit].str);
        addstr(buf);
        next = head->next;
        free(head);
        head = next;
    }
}


       
// Can change text size ??
// below text -> While background
// ps 파일체크 -> user 가변길이, CPU/MEM %5.1f, TTY 가변길이, COMMAND 나머지 채우기.
// disk partition 정보 체크 -> Filename 가변길이, mount path 가변길이, Usage: %5.1f, (Used / Total)
// warning history 출력 -> warning 기록 저장필요.
// Viewer Setting 변경