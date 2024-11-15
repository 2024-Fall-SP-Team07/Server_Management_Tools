#ifndef CPU_INFO_H
#define CPU_INFO_H

typedef struct CPUInfo {
    float temp;
    float avgTemp;
    int usage;
    int avgUsage;
} CPUInfo;

float get_CPU_Temperature();
float get_CPUInfo();
float get_CPU_avg_Usage();
#endif