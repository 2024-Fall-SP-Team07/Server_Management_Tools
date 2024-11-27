#include "cpu_info_from_log.h"
#include "mem_info_from_log.h"
#include "sys_info.h"
#include "disk_info.h"
#include "resources_display.h"
#include <stdio.h>
#include <stdlib.h>
#include <termio.h>
#include <curses.h>
#include <string.h>
#include <unistd.h>

const Unit_Mapping unitMap[] = {
    { KB, "KB" },
    { MB, "MB" },
    { GB, "GB" },
    { TB, "TB" }
};

void display_main(void){
    struct winsize wbuf;
    float now_row = 0, now_col = 0;
    int duration = 10, refresh_cycle = 1;
    short path_max_len = 0;
    CPU_Result cpu_info, avg_cpu_info;
    MEM_Info mem_info;
    UNIT MEM_unit = GB, SWAP_unit = MB, DISK_unit = GB;
    DISK_Result* head = NULL, *next = NULL;
    char buf[LEN] = { '\0' }, funcBuf[FUNC_BUF_LEN] = { '\0' };
    (ioctl(0, TIOCGWINSZ, &wbuf) == -1) ? printf("%s", exception(-4, "main", "Windows Size")) : 0;
    initscr();
    curs_set(0);
    move(1, (now_col = wbuf.ws_col * 0.01));
    // Print title
    sprintf(buf,"[Server Resources Monitor] (Refresh Cycle: %d seconds)", refresh_cycle);
    addstr(buf);
    // Print Server Hostname, IPv4 Address
    move((now_row = wbuf.ws_row * 0.1), (now_col = wbuf.ws_col * 0.01));
    get_Hostname(funcBuf);
    sprintf(buf, "Server: %s\t\t\t\t", funcBuf);
    addstr(buf);
    get_IPv4(funcBuf);
    sprintf(buf, "IP: %s\t\t", funcBuf);
    addstr(buf);
    move(now_row += 1, now_col);
    while(1){
        // Print System Time, Uptime
        get_SystemTime(funcBuf);
        sprintf(buf, "System Time: %s\t\t",funcBuf);
        addstr(buf);
        get_UpTime(funcBuf);
        sprintf(buf, "Uptime: %s", funcBuf);
        addstr(buf);
        
        cpu_info = get_CPU_Information(1);
        avg_cpu_info = get_CPU_Information(duration);

        // Print CPU Temperatrue
        move(now_row + 2, now_col);
        addstr("Temperature");
        move(now_row + 3, now_col);
        addstr("CPU:      [");
        for (int i = 0; i < wbuf.ws_col * BAR_RATIO; (i++ < (wbuf.ws_col*BAR_RATIO*(cpu_info.temp / 90))) ? addstr("#") : addstr(" "));
        sprintf(buf, "] %.1f Celsius", cpu_info.temp);
        addstr(buf);
        move(now_row + 4, now_col);
        addstr("CPU(AVG): [");
        for (int i = 0; i < wbuf.ws_col * BAR_RATIO; (i++ < (wbuf.ws_col*BAR_RATIO*(avg_cpu_info.temp / 90))) ? addstr("#") : addstr(" "));
        sprintf(buf, "] %.1f Celsius", avg_cpu_info.temp);
        addstr(buf);
        move(now_row + 4, wbuf.ws_col * (BAR_RATIO + 0.25));
        sprintf(buf, "(Duration: %d sec.)", duration);
        addstr(buf);

        // Print CPU Usage (instantaneous; %)
        move(now_row + 6, now_col);
        addstr("CPU / Memory Usage");
        move(now_row + 7, now_col);
        addstr("CPU:      [");
        for (int i = 0; i < wbuf.ws_col * BAR_RATIO; (i++ < (wbuf.ws_col*BAR_RATIO*(cpu_info.usage / 100))) ? addstr("#") : addstr(" "));
        sprintf(buf, "] %5.1f%%", cpu_info.usage);
        addstr(buf);

        // Print CPU Usage (Average during entered duration; %)
        move(now_row + 8, now_col);
        sprintf(buf, "CPU(AVG): [");
        addstr(buf);
        for (int i = 0; i < wbuf.ws_col * BAR_RATIO; (i++ < (wbuf.ws_col*BAR_RATIO*(avg_cpu_info.usage / 100))) ? addstr("#") : addstr(" "));
        sprintf(buf, "] %5.1f%%", avg_cpu_info.usage);
        addstr(buf);
        move(now_row + 8, wbuf.ws_col * (BAR_RATIO + 0.2));
        sprintf(buf, "(Duration: %d sec.)", duration);
        addstr(buf);

        // Print Physical Memeory Usage
        move(now_row + 9, now_col);
        addstr("MEM:      [");
        mem_info = get_Mem_Information(1);
        for (int i = 0; i < wbuf.ws_col * BAR_RATIO; (i++ < (wbuf.ws_col*BAR_RATIO*((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total))) ? addstr("#") : addstr(" "));
        sprintf(buf, "] %5.1lf%%", ((mem_info.size.mem_total - mem_info.size.mem_free) / (double)mem_info.size.mem_total) * 100);
        addstr(buf);
        move(now_row + 9, wbuf.ws_col * (BAR_RATIO + 0.2));
        sprintf(buf, "(%.2lf%s / %.2lf%s)", convert_Size_Unit(mem_info.size.mem_total - mem_info.size.mem_free, MEM_unit), unitMap[(int)MEM_unit].str, convert_Size_Unit(mem_info.size.mem_total, MEM_unit), unitMap[(int)MEM_unit].str);
        addstr(buf);

        // Print Swap Memroy Usage
        move(now_row + 10, now_col);
        addstr("SWAP:     [");
        for (int i = 0; i < wbuf.ws_col * BAR_RATIO; (i++ < (wbuf.ws_col*BAR_RATIO*((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total))) ? addstr("#") : addstr(" "));
        sprintf(buf, "] %5.1lf%%", ((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100);
        addstr(buf);
        move(now_row + 10, wbuf.ws_col * (BAR_RATIO + 0.2));
        sprintf(buf, "(%.2lf%s / %.2lf%s)", convert_Size_Unit(mem_info.size.swap_total - mem_info.size.swap_free, SWAP_unit), unitMap[(int)SWAP_unit].str, convert_Size_Unit(mem_info.size.swap_total, SWAP_unit), unitMap[(int)SWAP_unit].str);
        addstr(buf);
        
        // Print Disk Usage
        move (now_row + 12, now_col);
        addstr("Disk Usage");
        head = get_Partition_Info_List();
        path_max_len = get_Partition_Max_Length(head);
        for (int i = 13; head != NULL; i++){
            move (now_row + i, now_col);
            sprintf(buf, "%s: ", head->mount_path);
            addstr(buf);
            move (now_row + i, now_col + path_max_len + 1);
            addstr(" [");
            for (int i = path_max_len * 0.5; i < wbuf.ws_col * BAR_RATIO; (i++ < (wbuf.ws_col*BAR_RATIO*((head->size.total_space - head->size.free_size) / (double)head->size.total_space))) ? addstr("#") : addstr(" "));
            sprintf(buf, "] %5.1lf%%", ((head->size.total_space - head->size.free_size) / (double)head->size.total_space) * 100);
            addstr(buf);
            move(now_row  + i, wbuf.ws_col * (BAR_RATIO + 0.2));
            sprintf(buf, "(%.2lf%s / %.2lf%s)", convert_Size_Unit(head->size.total_space - head->size.free_size, DISK_unit), unitMap[(int)DISK_unit].str, convert_Size_Unit(head->size.total_space, DISK_unit), unitMap[(int)DISK_unit].str);
            addstr(buf);
            next = head->next;
            free(head);
            head = next;
        }
        
        move(now_row, now_col);
        refresh();
        sleep(refresh_cycle);
    }
    getch();
    endwin();
}
