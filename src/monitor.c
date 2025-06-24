#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "../include/monitor.h"
#include "../include/utils.h"
#include "../include/cgroup.h"


struct ebpf_event {
    uint64_t timestamp;
    uint32_t pid;
    uint32_t uid;
    uint8_t event_type;
    char data[256];
};


enum event_types {
    EVENT_SYSCALL = 1,
    EVENT_NAMESPACE = 2,
    EVENT_CGROUP = 3
};


static int bpf_prog_fd = -1;
static int bpf_map_fd = -1;


#define LOG_BASE_PATH "/var/lib/simplecontainer/logs"


int monitor_init() {
    
    if (!directory_exists(LOG_BASE_PATH)) {
        if (create_directory(LOG_BASE_PATH, 0755) != 0) {
            log_error("خطا در ایجاد دایرکتوری لاگ");
            return -1;
        }
    }
    
    log_message("مانیتورینگ eBPF راه‌اندازی شد");
    return 0;
}


int monitor_cleanup() {
    if (bpf_prog_fd >= 0) {
        close(bpf_prog_fd);
        bpf_prog_fd = -1;
    }
    
    if (bpf_map_fd >= 0) {
        close(bpf_map_fd);
        bpf_map_fd = -1;
    }
    
    log_message("مانیتورینگ eBPF پاک‌سازی شد");
    return 0;
}


static int log_event(const char *container_id, uint8_t event_type, const char *format, ...) {
    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/%s.log", LOG_BASE_PATH, container_id);
    
    FILE *log_file = fopen(log_path, "a");
    if (!log_file) {
        log_error("خطا در باز کردن فایل لاگ: %s", log_path);
        return -1;
    }
    
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", tm_info);
    
    
    const char *event_type_str;
    switch (event_type) {
        case EVENT_SYSCALL:
            event_type_str = "SYSCALL";
            break;
        case EVENT_NAMESPACE:
            event_type_str = "NAMESPACE";
            break;
        case EVENT_CGROUP:
            event_type_str = "CGROUP";
            break;
        default:
            event_type_str = "UNKNOWN";
    }
    
    
    fprintf(log_file, "%s %s: ", timestamp, event_type_str);
    
    
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    
    fprintf(log_file, "\n");
    fclose(log_file);
    
    return 0;
}


int monitor_container(container_config_t *config) {
    
    log_event(config->id, EVENT_CGROUP, "container started with PID %d", config->container_pid);
    
    
    log_event(config->id, EVENT_NAMESPACE, "created PID namespace");
    log_event(config->id, EVENT_NAMESPACE, "created UTS namespace (hostname: %s)", config->name);
    log_event(config->id, EVENT_NAMESPACE, "created mount namespace");
    log_event(config->id, EVENT_NAMESPACE, "created user namespace");
    log_event(config->id, EVENT_NAMESPACE, "created network namespace");
    log_event(config->id, EVENT_NAMESPACE, "created IPC namespace");
    
    
    log_event(config->id, EVENT_CGROUP, "memory limit set to %lu bytes", config->mem_limit_bytes);
    log_event(config->id, EVENT_CGROUP, "CPU shares set to %lu", config->cpu_shares);
    
    if (config->cpu_affinity >= 0) {
        log_event(config->id, EVENT_CGROUP, "CPU affinity set to core %d", config->cpu_affinity);
    }
    
    log_event(config->id, EVENT_CGROUP, "I/O weight set to %lu", config->io_weight);
    
    
    log_event(config->id, EVENT_SYSCALL, "execve (pid=%d, binary=\"%s\")", 
              config->container_pid, config->binary_path);
    
    return 0;
}


int monitor_stop_container(container_config_t *config) {
    
    log_event(config->id, EVENT_CGROUP, "container stopped");
    
    return 0;
}


int monitor_get_resource_usage(container_config_t *config, uint64_t *cpu_usage, uint64_t *mem_usage, uint64_t *io_read, uint64_t *io_write) {
    
    
    
    
    if (cgroup_get_memory_usage(config, mem_usage) != 0) {
        return -1;
    }
    
    
    if (cgroup_get_cpu_usage(config, cpu_usage) != 0) {
        return -1;
    }
    
    
    if (cgroup_get_io_usage(config, io_read, io_write) != 0) {
        return -1;
    }
    
    return 0;
}


int monitor_namespace_events() {
    return 0;
}

int monitor_cgroup_events() {
    return 0;
}

int monitor_syscall_events() {
    return 0;
}

int monitor_save_logs(const char *container_id) {
    (void)container_id;
    return 0;
}