#define _POSIX_C_SOURCE 200809L

#include "sysinfo.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>

static char *trim(char *text)
{
    char *end;

    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') {
        text++;
    }

    end = text + strlen(text);
    while (end > text &&
           (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' ||
            end[-1] == '\n')) {
        *--end = '\0';
    }

    return text;
}

static int copy_value(char *output, size_t output_size, const char *value)
{
    int written = snprintf(output, output_size, "%s", value);

    if (written < 0 || (size_t)written >= output_size) {
        errno = ENOSPC;
        return -1;
    }

    return 0;
}

int parse_os_release(FILE *stream, char *output, size_t output_size)
{
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int result = -1;

    if (stream == NULL || output == NULL || output_size == 0) {
        errno = EINVAL;
        return -1;
    }

    errno = 0;
    while ((length = getline(&line, &capacity, stream)) >= 0) {
        char *value;
        size_t value_length;

        (void)length;
        if (strncmp(line, "PRETTY_NAME=", sizeof("PRETTY_NAME=") - 1) != 0) {
            continue;
        }

        value = trim(line + sizeof("PRETTY_NAME=") - 1);
        value_length = strlen(value);
        if (value_length >= 2 &&
            ((value[0] == '"' && value[value_length - 1] == '"') ||
             (value[0] == '\'' && value[value_length - 1] == '\''))) {
            value[value_length - 1] = '\0';
            value++;
        }

        result = copy_value(output, output_size, value);
        break;
    }

    free(line);
    if (result != 0 && errno == 0) {
        errno = ENOENT;
    }
    return result;
}

int parse_cpu_model(FILE *stream, char *output, size_t output_size)
{
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int result = -1;

    if (stream == NULL || output == NULL || output_size == 0) {
        errno = EINVAL;
        return -1;
    }

    errno = 0;
    while ((length = getline(&line, &capacity, stream)) >= 0) {
        char *separator = strchr(line, ':');
        char *key;

        (void)length;
        if (separator == NULL) {
            continue;
        }

        *separator = '\0';
        key = trim(line);
        if (strcmp(key, "model name") != 0 && strcmp(key, "Hardware") != 0 &&
            strcmp(key, "Model") != 0) {
            continue;
        }

        result = copy_value(output, output_size, trim(separator + 1));
        break;
    }

    free(line);
    if (result != 0 && errno == 0) {
        errno = ENOENT;
    }
    return result;
}

int parse_memory_info(FILE *stream, char *output, size_t output_size)
{
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    unsigned long long total_kib = 0;
    unsigned long long available_kib = 0;

    if (stream == NULL || output == NULL || output_size == 0) {
        errno = EINVAL;
        return -1;
    }

    while ((length = getline(&line, &capacity, stream)) >= 0) {
        unsigned long long value;
        char key[32];

        (void)length;
        if (sscanf(line, "%31[^:]: %llu kB", key, &value) != 2) {
            continue;
        }
        if (strcmp(key, "MemTotal") == 0) {
            total_kib = value;
        } else if (strcmp(key, "MemAvailable") == 0) {
            available_kib = value;
        }
    }

    free(line);
    if (total_kib == 0 || available_kib > total_kib) {
        errno = ENODATA;
        return -1;
    }

    {
        double total_gib = (double)total_kib / (1024.0 * 1024.0);
        double used_gib =
            (double)(total_kib - available_kib) / (1024.0 * 1024.0);
        int written = snprintf(output, output_size, "%.1f GiB / %.1f GiB",
                               used_gib, total_gib);

        if (written < 0 || (size_t)written >= output_size) {
            errno = ENOSPC;
            return -1;
        }
    }

    return 0;
}

int format_uptime(double seconds, char *output, size_t output_size)
{
    unsigned long long total_minutes;
    unsigned long long days;
    unsigned long long hours;
    unsigned long long minutes;
    int written;

    if (output == NULL || output_size == 0 || !(seconds >= 0.0) ||
        seconds / 60.0 >= (double)ULLONG_MAX) {
        errno = EINVAL;
        return -1;
    }

    total_minutes = (unsigned long long)(seconds / 60.0);
    days = total_minutes / (24 * 60);
    hours = (total_minutes / 60) % 24;
    minutes = total_minutes % 60;
    if (days > 0) {
        written = snprintf(output, output_size, "%llu day%s, %02llu:%02llu",
                           days, days == 1 ? "" : "s", hours, minutes);
    } else {
        written = snprintf(output, output_size, "%02llu:%02llu", hours,
                           minutes);
    }

    if (written < 0 || (size_t)written >= output_size) {
        errno = ENOSPC;
        return -1;
    }

    return 0;
}

static void warn_unavailable(const char *item, const char *source)
{
    int saved_errno = errno;

    (void)fprintf(stderr, "kdlfetch: warning: cannot read %s from %s: %s\n",
                  item, source, strerror(saved_errno));
    errno = saved_errno;
}

static int read_file(const char *path, int (*parser)(FILE *, char *, size_t),
                    char *output, size_t output_size)
{
    FILE *stream = fopen(path, "r");

    if (stream == NULL) {
        return -1;
    }

    if (parser(stream, output, output_size) != 0) {
        int saved_errno = errno;

        (void)fclose(stream);
        errno = saved_errno;
        return -1;
    }

    if (fclose(stream) != 0) {
        return -1;
    }
    return 0;
}

static int collect_uptime(char *output, size_t output_size)
{
    FILE *stream = fopen("/proc/uptime", "r");
    double seconds;

    if (stream == NULL) {
        return -1;
    }
    if (fscanf(stream, "%lf", &seconds) != 1) {
        int saved_errno = errno == 0 ? EINVAL : errno;

        (void)fclose(stream);
        errno = saved_errno;
        return -1;
    }
    if (fclose(stream) != 0) {
        return -1;
    }
    return format_uptime(seconds, output, output_size);
}

static int collect_disk(char *output, size_t output_size)
{
    struct statvfs stats;
    double total_gib;
    double used_gib;
    int written;

    if (statvfs("/", &stats) != 0) {
        return -1;
    }

    total_gib =
        (double)stats.f_blocks * (double)stats.f_frsize / (1024.0 * 1024.0 *
                                                            1024.0);
    used_gib = (double)(stats.f_blocks - stats.f_bavail) *
               (double)stats.f_frsize / (1024.0 * 1024.0 * 1024.0);
    written = snprintf(output, output_size, "%.1f GiB / %.1f GiB",
                       used_gib, total_gib);
    if (written < 0 || (size_t)written >= output_size) {
        errno = ENOSPC;
        return -1;
    }
    return 0;
}

static void mark_unavailable(struct SystemInfo *info, const char *item,
                             const char *source, char *output,
                             size_t output_size)
{
    warn_unavailable(item, source);
    if (copy_value(output, output_size, "Unavailable") != 0) {
        (void)fprintf(stderr, "kdlfetch: error: output buffer too small\n");
    }
    info->incomplete = 1;
}

int collect_system_info(struct SystemInfo *info)
{
    struct utsname system_name;
    int hostname_result;

    if (info == NULL) {
        errno = EINVAL;
        return -1;
    }

    info->incomplete = 0;
    if (read_file("/etc/os-release", parse_os_release, info->distro,
                  sizeof(info->distro)) != 0) {
        mark_unavailable(info, "distribution", "/etc/os-release", info->distro,
                         sizeof(info->distro));
    }
    if (uname(&system_name) == 0) {
        if (copy_value(info->kernel, sizeof(info->kernel), system_name.release) !=
            0) {
            mark_unavailable(info, "kernel", "uname", info->kernel,
                             sizeof(info->kernel));
        }
    } else {
        mark_unavailable(info, "kernel", "uname", info->kernel,
                         sizeof(info->kernel));
    }

    hostname_result = gethostname(info->hostname, sizeof(info->hostname));
    if (hostname_result != 0 ||
        memchr(info->hostname, '\0', sizeof(info->hostname)) == NULL) {
        if (hostname_result == 0) {
            errno = ENAMETOOLONG;
        }
        mark_unavailable(info, "hostname", "gethostname", info->hostname,
                         sizeof(info->hostname));
    }

    if (collect_uptime(info->uptime, sizeof(info->uptime)) != 0) {
        mark_unavailable(info, "uptime", "/proc/uptime", info->uptime,
                         sizeof(info->uptime));
    }
    if (read_file("/proc/cpuinfo", parse_cpu_model, info->cpu,
                  sizeof(info->cpu)) != 0) {
        mark_unavailable(info, "CPU model", "/proc/cpuinfo", info->cpu,
                         sizeof(info->cpu));
    }
    if (read_file("/proc/meminfo", parse_memory_info, info->memory,
                  sizeof(info->memory)) != 0) {
        mark_unavailable(info, "memory", "/proc/meminfo", info->memory,
                         sizeof(info->memory));
    }
    if (collect_disk(info->disk, sizeof(info->disk)) != 0) {
        mark_unavailable(info, "disk usage", "/", info->disk,
                         sizeof(info->disk));
    }

    return info->incomplete ? 1 : 0;
}

void print_system_info(const struct SystemInfo *info)
{
    static const char *const logo[] = {
        "  _  _",
        " | || |",
        "  \\_, |",
        "  |__/",
    };
    char details[7][320];
    size_t index;

    (void)snprintf(details[0], sizeof(details[0]), "OS       : %s", info->distro);
    (void)snprintf(details[1], sizeof(details[1]), "Kernel   : %s", info->kernel);
    (void)snprintf(details[2], sizeof(details[2]), "Hostname : %s", info->hostname);
    (void)snprintf(details[3], sizeof(details[3]), "Uptime   : %s", info->uptime);
    (void)snprintf(details[4], sizeof(details[4]), "CPU      : %s", info->cpu);
    (void)snprintf(details[5], sizeof(details[5]), "Memory   : %s", info->memory);
    (void)snprintf(details[6], sizeof(details[6]), "Disk     : %s", info->disk);

    for (index = 0; index < sizeof(details) / sizeof(details[0]); index++) {
        const char *logo_line = index < sizeof(logo) / sizeof(logo[0])
                                    ? logo[index]
                                    : "";

        (void)printf("%-12s%s\n", logo_line, details[index]);
    }
}
