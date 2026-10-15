#include "sysinfo.h"

#include <stdlib.h>

int main(void)
{
    struct SystemInfo info;
    int result = collect_system_info(&info);

    print_system_info(&info);
    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
