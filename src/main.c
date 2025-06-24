#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/container.h"
#include "../include/cli.h"
#include "../include/monitor.h"
#include "../include/utils.h"

#define MAX_CONTAINERS 100

int main(int argc, char **argv) {
    
    if (!has_root_privileges()) {
        fprintf(stderr, "این برنامه نیاز به دسترسی root دارد\n");
        return EXIT_FAILURE;
    }

    
    if (monitor_init() != 0) {
        fprintf(stderr, "خطا در راه‌اندازی مانیتورینگ eBPF\n");
        return EXIT_FAILURE;
    }

    
    container_manager_t *manager = container_manager_create(MAX_CONTAINERS);
    if (!manager) {
        fprintf(stderr, "خطا در ایجاد مدیریت‌کننده کانتینر\n");
        monitor_cleanup();
        return EXIT_FAILURE;
    }

    
    int result = cli_process_command(manager, argc, argv);

    
    container_manager_destroy(manager);
    monitor_cleanup();

    return result;
}