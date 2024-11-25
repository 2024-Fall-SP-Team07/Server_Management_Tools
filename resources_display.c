#include "cpu_info_from_log.h"
#include "mem_info_from_log.h"
#include "sys_info.h"
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
    CPU_Result cpu_info, avg_cpu_info;
    MEM_Info mem_info;
    UNIT unit = KB;
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
        sprintf(buf, "(%.2lf%s / %.2lf%s)", convert_Size_Unit(mem_info.size.mem_total - mem_info.size.mem_free, unit), unitMap[(int)unit].str, convert_Size_Unit(mem_info.size.mem_total, unit), unitMap[(int)unit].str);
        addstr(buf);

        // Print Swap Memroy Usage
        move(now_row + 10, now_col);
        addstr("SWAP:     [");
        for (int i = 0; i < wbuf.ws_col * BAR_RATIO; (i++ < (wbuf.ws_col*BAR_RATIO*((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total))) ? addstr("#") : addstr(" "));
        sprintf(buf, "] %5.1lf%%", ((mem_info.size.swap_total - mem_info.size.swap_free) / (double)mem_info.size.swap_total) * 100);
        addstr(buf);
        move(now_row + 10, wbuf.ws_col * (BAR_RATIO + 0.2));
        sprintf(buf, "(%.2lf%s / %.2lf%s)", convert_Size_Unit(mem_info.size.swap_total - mem_info.size.swap_free, unit), unitMap[(int)unit].str, convert_Size_Unit(mem_info.size.swap_total, unit), unitMap[(int)unit].str);
        addstr(buf);
        
        // Print Disk Usage
        move (now_row + 12, now_col);
        addstr("Disk Usage");


        move(now_row, now_col);
        refresh();
        sleep(refresh_cycle);
    }
    getch();
    endwin();
}
