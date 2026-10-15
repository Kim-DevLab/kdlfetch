#ifndef KDLFETCH_SYSINFO_H
#define KDLFETCH_SYSINFO_H

#include <stddef.h>
#include <stdio.h>

struct SystemInfo {
    char distro[128];
    char kernel[128];
    char hostname[256];
    char uptime[128];
    char cpu[256];
    char memory[128];
    char disk[128];
    int incomplete;
};

int parse_os_release(FILE *stream, char *output, size_t output_size);
int parse_cpu_model(FILE *stream, char *output, size_t output_size);
int parse_memory_info(FILE *stream, char *output, size_t output_size);
int format_uptime(double seconds, char *output, size_t output_size);
int collect_system_info(struct SystemInfo *info);
void print_system_info(const struct SystemInfo *info);

#endif
