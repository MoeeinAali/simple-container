#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../include/cgroup.h"
#include "../include/utils.h"


int cgroup_setup(container_config_t *config) {
    
    if (!directory_exists(CGROUP_BASE_PATH)) {
        if (create_directory(CGROUP_BASE_PATH, 0755) != 0) {
            log_error("خطا در ایجاد دایرکتوری پایه cgroup");
            return -1;
        }
    }
    
    
    char cgroup_full_path[512];
    snprintf(cgroup_full_path, sizeof(cgroup_full_path), "%s/%s", CGROUP_BASE_PATH, config->id);
    
    
    strncpy(config->cgroup_path, cgroup_full_path, sizeof(config->cgroup_path) - 1);
    
    
    if (create_directory(cgroup_full_path, 0755) != 0) {
        log_error("خطا در ایجاد دایرکتوری cgroup برای کانتینر");
        return -1;
    }
    
    
    if (write_cgroup_file(CGROUP_BASE_PATH, "cgroup.subtree_control", "+memory +cpu +io") != 0) {
        log_error("خطا در فعال‌سازی کنترلرهای cgroup");
        return -1;
    }
    
    log_message("cgroup برای کانتینر %s ایجاد شد در %s", config->id, cgroup_full_path);
    return 0;
}


int cgroup_cleanup(container_config_t *config) {
    
    if (remove_directory(config->cgroup_path) != 0) {
        log_error("خطا در حذف دایرکتوری cgroup");
        return -1;
    }
    
    log_message("cgroup برای کانتینر %s پاک‌سازی شد", config->id);
    return 0;
}


int write_cgroup_file(const char *cgroup_path, const char *file, const char *value) {
    
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", cgroup_path, file);
    
    
    int fd = open(file_path, O_WRONLY);
    if (fd == -1) {
        log_error("خطا در باز کردن فایل cgroup: %s", file_path);
        return -1;
    }
    
    
    ssize_t bytes_written = write(fd, value, strlen(value));
    close(fd);
    
    if (bytes_written != (ssize_t)strlen(value)) {
        log_error("خطا در نوشتن به فایل cgroup: %s", file_path);
        return -1;
    }
    
    return 0;
}


int read_cgroup_file(const char *cgroup_path, const char *file, char *buffer, size_t buffer_size) {
    
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s/%s", cgroup_path, file);
    
    
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        log_error("خطا در باز کردن فایل cgroup: %s", file_path);
        return -1;
    }
    
    
    ssize_t bytes_read = read(fd, buffer, buffer_size - 1);
    close(fd);
    
    if (bytes_read == -1) {
        log_error("خطا در خواندن از فایل cgroup: %s", file_path);
        return -1;
    }
    
    buffer[bytes_read] = '\0';
    return 0;
}


int cgroup_set_memory_limit(container_config_t *config, uint64_t limit_bytes) {
    
    char limit_str[32];
    snprintf(limit_str, sizeof(limit_str), "%lu", limit_bytes);
    
    if (write_cgroup_file(config->cgroup_path, "memory.max", limit_str) != 0) {
        log_error("خطا در تنظیم محدودیت حافظه");
        return -1;
    }
    
    
    config->mem_limit_bytes = limit_bytes;
    
    log_message("محدودیت حافظه برای کانتینر %s تنظیم شد: %lu bytes", config->id, limit_bytes);
    return 0;
}


int cgroup_set_cpu_shares(container_config_t *config, uint64_t shares) {
    
    char shares_str[32];
    snprintf(shares_str, sizeof(shares_str), "%lu", shares);
    
    if (write_cgroup_file(config->cgroup_path, "cpu.weight", shares_str) != 0) {
        log_error("خطا در تنظیم سهم CPU");
        return -1;
    }
    
    
    config->cpu_shares = shares;
    
    log_message("سهم CPU برای کانتینر %s تنظیم شد: %lu", config->id, shares);
    return 0;
}


int cgroup_set_cpu_affinity(container_config_t *config, int cpu_id) {
    if (cpu_id < 0) {
        return 0;  
    }
    
    
    char cpuset_str[256] = {0};
    snprintf(cpuset_str, sizeof(cpuset_str), "%d", cpu_id);
    
    if (write_cgroup_file(config->cgroup_path, "cpuset.cpus", cpuset_str) != 0) {
        log_error("خطا در تنظیم تخصیص CPU");
        return -1;
    }
    
    
    config->cpu_affinity = cpu_id;
    
    log_message("تخصیص CPU برای کانتینر %s تنظیم شد: CPU %d", config->id, cpu_id);
    return 0;
}


int cgroup_set_io_weight(container_config_t *config, uint64_t weight) {
    
    char weight_str[32];
    snprintf(weight_str, sizeof(weight_str), "%lu", weight);
    
    if (write_cgroup_file(config->cgroup_path, "io.weight", weight_str) != 0) {
        log_error("خطا در تنظیم وزن I/O");
        return -1;
    }
    
    
    config->io_weight = weight;
    
    log_message("وزن I/O برای کانتینر %s تنظیم شد: %lu", config->id, weight);
    return 0;
}


int cgroup_add_process(container_config_t *config, pid_t pid) {
    
    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d", pid);
    
    
    if (write_cgroup_file(config->cgroup_path, "cgroup.procs", pid_str) != 0) {
        log_error("خطا در اضافه کردن فرآیند به cgroup");
        return -1;
    }
    
    log_message("فرآیند %d به cgroup کانتینر %s اضافه شد", pid, config->id);
    return 0;
}


int cgroup_get_memory_usage(container_config_t *config, uint64_t *usage) {
    char buffer[128];
    if (read_cgroup_file(config->cgroup_path, "memory.current", buffer, sizeof(buffer)) != 0) {
        log_error("خطا در خواندن مصرف حافظه");
        return -1;
    }
    
    *usage = strtoull(buffer, NULL, 10);
    return 0;
}


int cgroup_get_cpu_usage(container_config_t *config, uint64_t *usage) {
    char buffer[512];
    if (read_cgroup_file(config->cgroup_path, "cpu.stat", buffer, sizeof(buffer)) != 0) {
        log_error("خطا در خواندن مصرف CPU");
        return -1;
    }
    
    
    char *line = strtok(buffer, "\n");
    while (line) {
        if (strncmp(line, "usage_usec ", 11) == 0) {
            *usage = strtoull(line + 11, NULL, 10);
            return 0;
        }
        line = strtok(NULL, "\n");
    }
    
    *usage = 0;
    return 0;
}


int cgroup_get_io_usage(container_config_t *config, uint64_t *read_bytes, uint64_t *write_bytes) {
    char buffer[1024];
    if (read_cgroup_file(config->cgroup_path, "io.stat", buffer, sizeof(buffer)) != 0) {
        log_error("خطا در خواندن مصرف I/O");
        return -1;
    }
    
    *read_bytes = 0;
    *write_bytes = 0;
    
    
    char *line = strtok(buffer, "\n");
    while (line) {
        if (strstr(line, "rbytes=")) {
            char *rbytes_str = strstr(line, "rbytes=") + 7;
            *read_bytes += strtoull(rbytes_str, NULL, 10);
        }
        if (strstr(line, "wbytes=")) {
            char *wbytes_str = strstr(line, "wbytes=") + 7;
            *write_bytes += strtoull(wbytes_str, NULL, 10);
        }
        line = strtok(NULL, "\n");
    }
    
    return 0;
}