#include "../src/sysinfo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            (void)fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__,     \
                          #condition);                                         \
            return EXIT_FAILURE;                                              \
        }                                                                      \
    } while (0)

static FILE *stream_from_text(const char *text)
{
    FILE *stream = tmpfile();

    if (stream == NULL) {
        return NULL;
    }
    if (fputs(text, stream) == EOF || fflush(stream) != 0 ||
        fseek(stream, 0, SEEK_SET) != 0) {
        (void)fclose(stream);
        return NULL;
    }
    return stream;
}

static int test_os_release(void)
{
    char output[128];
    FILE *stream =
        stream_from_text("NAME=Debian\nPRETTY_NAME=\"Debian GNU/Linux 13 "
                         "(trixie)\"\n");

    CHECK(stream != NULL);
    CHECK(parse_os_release(stream, output, sizeof(output)) == 0);
    CHECK(strcmp(output, "Debian GNU/Linux 13 (trixie)") == 0);
    CHECK(fclose(stream) == 0);

    stream = stream_from_text("NAME=Debian\n");
    CHECK(stream != NULL);
    CHECK(parse_os_release(stream, output, sizeof(output)) != 0);
    CHECK(fclose(stream) == 0);
    return EXIT_SUCCESS;
}

static int test_cpu_model(void)
{
    char output[128];
    FILE *stream = stream_from_text(
        "processor : 0\nmodel name : Example CPU @ 3.00GHz\n");

    CHECK(stream != NULL);
    CHECK(parse_cpu_model(stream, output, sizeof(output)) == 0);
    CHECK(strcmp(output, "Example CPU @ 3.00GHz") == 0);
    CHECK(fclose(stream) == 0);
    return EXIT_SUCCESS;
}

static int test_memory_info(void)
{
    char output[128];
    FILE *stream =
        stream_from_text("MemTotal:       4194304 kB\nMemAvailable: "
                         "2097152 kB\n");

    CHECK(stream != NULL);
    CHECK(parse_memory_info(stream, output, sizeof(output)) == 0);
    CHECK(strcmp(output, "2.0 GiB / 4.0 GiB") == 0);
    CHECK(fclose(stream) == 0);
    return EXIT_SUCCESS;
}

static int test_uptime(void)
{
    char output[128];

    CHECK(format_uptime(0.0, output, sizeof(output)) == 0);
    CHECK(strcmp(output, "00:00") == 0);
    CHECK(format_uptime(90061.0, output, sizeof(output)) == 0);
    CHECK(strcmp(output, "1 day, 01:01") == 0);
    return EXIT_SUCCESS;
}

int main(void)
{
    if (test_os_release() != EXIT_SUCCESS ||
        test_cpu_model() != EXIT_SUCCESS ||
        test_memory_info() != EXIT_SUCCESS || test_uptime() != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    (void)puts("All kdlfetch tests passed.");
    return EXIT_SUCCESS;
}
