#include <sched.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sched.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sched.h>
#include "../include/container.h"
#include "../include/namespace.h"
#include "../include/cgroup.h"
#include "../include/filesystem.h"
#include "../include/monitor.h"
#include "../include/utils.h"


container_manager_t* container_manager_create(int max_containers) {
    container_manager_t *manager = malloc(sizeof(container_manager_t));
    if (!manager) {
        log_error("خطا در تخصیص حافظه برای مدیریت‌کننده کانتینر");
        return NULL;
    }

    manager->containers = malloc(sizeof(container_config_t) * max_containers);
    if (!manager->containers) {
        log_error("خطا در تخصیص حافظه برای آرایه کانتینرها");
        free(manager);
        return NULL;
    }

    manager->max_containers = max_containers;
    manager->container_count = 0;

    
    create_directory("/var/lib/simplecontainer", 0755);
    create_directory("/var/lib/simplecontainer/rootfs", 0755);
    create_directory("/var/lib/simplecontainer/overlays", 0755);
    create_directory("/var/lib/simplecontainer/logs", 0755);

    return manager;
}


void container_manager_destroy(container_manager_t *manager) {
    if (!manager) return;

    
    for (int i = 0; i < manager->container_count; i++) {
        if (manager->containers[i].running) {
            container_stop(manager, manager->containers[i].id);
        }
    }

    free(manager->containers);
    free(manager);
}


int container_create(container_manager_t *manager, const char *name, const char *binary_path, char **args, int argc) {
    if (manager->container_count >= manager->max_containers) {
        log_error("تعداد کانتینرها به حداکثر رسیده است");
        return -1;
    }

    container_config_t *config = &manager->containers[manager->container_count];
    memset(config, 0, sizeof(container_config_t));

    
    generate_unique_id(config->id, sizeof(config->id));
    strncpy(config->name, name, sizeof(config->name) - 1);
    
    
    snprintf(config->rootfs, sizeof(config->rootfs), "/var/lib/simplecontainer/rootfs/%s", config->id);
    snprintf(config->overlay_workdir, sizeof(config->overlay_workdir), "/var/lib/simplecontainer/overlays/%s", config->id);
    strncpy(config->binary_path, binary_path, sizeof(config->binary_path) - 1);
    
    
    config->args = malloc((argc + 1) * sizeof(char*));
    if (!config->args) {
        log_error("خطا در تخصیص حافظه برای آرگومان‌ها");
        return -1;
    }
    
    for (int i = 0; i < argc; i++) {
        config->args[i] = strdup(args[i]);
    }
    config->args[argc] = NULL;
    config->argc = argc;
    
    
    config->mem_limit_bytes = 512 * 1024 * 1024;  
    config->cpu_shares = 1024;                     
    config->cpu_affinity = -1;                     
    config->io_weight = 100;                       
    
    config->running = false;
    config->container_pid = -1;
    
    
    snprintf(config->cgroup_path, sizeof(config->cgroup_path), "/simplecontainer/%s", config->id);
    
    
    if (setup_container_rootfs(config) != 0) {
        log_error("خطا در آماده‌سازی فایل‌سیستم کانتینر");
        free(config->args);
        return -1;
    }
    
    manager->container_count++;
    log_message("کانتینر با شناسه %s ایجاد شد", config->id);
    
    return 0;
}


static int container_process(void *arg) {
    container_config_t *config = (container_config_t *)arg;
    
    
    if (setup_namespaces(config) != 0) {
        log_error("خطا در تنظیم namespace‌ها");
        return EXIT_FAILURE;
    }
    
    
    if (do_chroot(config->rootfs) != 0) {
        log_error("خطا در تنظیم chroot");
        return EXIT_FAILURE;
    }
    
    
    if (chdir("/") != 0) {
        log_error("خطا در تغییر دایرکتوری به /");
        return EXIT_FAILURE;
    }
    
    
    if (mount_essential_filesystems(config) != 0) {
        log_error("خطا در نصب فایل‌سیستم‌های ضروری");
        return EXIT_FAILURE;
    }
    
    
    execv(config->binary_path, config->args);
    
    
    log_error("خطا در اجرای برنامه: %s", config->binary_path);
    return EXIT_FAILURE;
}


int container_start(container_manager_t *manager, const char *container_id) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    if (config->running) {
        log_error("کانتینر %s در حال اجرا است", container_id);
        return -1;
    }
    
    
    if (cgroup_setup(config) != 0) {
        log_error("خطا در تنظیم cgroup");
        return -1;
    }
    
    
    cgroup_set_memory_limit(config, config->mem_limit_bytes);
    cgroup_set_cpu_shares(config, config->cpu_shares);
    if (config->cpu_affinity >= 0) {
        cgroup_set_cpu_affinity(config, config->cpu_affinity);
    }
    cgroup_set_io_weight(config, config->io_weight);
    
    
    const int stack_size = 8 * 1024 * 1024;  
    void *stack = malloc(stack_size);
    if (!stack) {
        log_error("خطا در تخصیص حافظه برای استک");
        return -1;
    }
    
    
    pid_t pid = clone(container_process, stack + stack_size,
                      CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWUSER | CLONE_NEWNET | CLONE_NEWIPC,
                      config);
    
    if (pid == -1) {
        log_error("خطا در ایجاد فرآیند کانتینر");
        free(stack);
        return -1;
    }
    
    
    char pid_str[16];
    snprintf(pid_str, sizeof(pid_str), "%d", pid);
    char cgroup_procs_path[512];
    snprintf(cgroup_procs_path, sizeof(cgroup_procs_path), "/sys/fs/cgroup/unified/%s/cgroup.procs", config->id);    
    int fd = open(cgroup_procs_path, O_WRONLY);
    if (fd == -1) {
        log_error("خطا در باز کردن فایل cgroup.procs");
        kill(pid, SIGKILL);
        free(stack);
        return -1;
    }
    
    write(fd, pid_str, strlen(pid_str));
    close(fd);
    
    
    config->container_pid = pid;
    config->running = true;
    
    
    monitor_container(config);
    
    free(stack);
    log_message("کانتینر %s با PID %d شروع شد", container_id, pid);
    
    return 0;
}


int container_stop(container_manager_t *manager, const char *container_id) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    if (!config->running) {
        log_error("کانتینر %s در حال اجرا نیست", container_id);
        return -1;
    }
    
    
    if (kill(config->container_pid, SIGTERM) == -1) {
        log_error("خطا در ارسال سیگنال SIGTERM به کانتینر");
        return -1;
    }
    
    
    int status;
    time_t start_time = time(NULL);
    while (time(NULL) - start_time < 10) {  
        if (waitpid(config->container_pid, &status, WNOHANG) != 0) {
            break;
        }
        usleep(100000);  
    }
    
    
    if (kill(config->container_pid, 0) == 0) {
        log_message("ارسال SIGKILL به کانتینر %s", container_id);
        kill(config->container_pid, SIGKILL);
        waitpid(config->container_pid, &status, 0);
    }
    
    
    monitor_stop_container(config);
    
    
    cgroup_cleanup(config);
    
    
    config->container_pid = -1;
    config->running = false;
    
    log_message("کانتینر %s متوقف شد", container_id);
    
    return 0;
}


int container_status(container_manager_t *manager, const char *container_id) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    printf("شناسه: %s\n", config->id);
    printf("نام: %s\n", config->name);
    printf("وضعیت: %s\n", config->running ? "در حال اجرا" : "متوقف");
    
    if (config->running) {
        printf("PID: %d\n", config->container_pid);
        
        
        uint64_t cpu_usage, mem_usage, io_read, io_write;
        if (monitor_get_resource_usage(config, &cpu_usage, &mem_usage, &io_read, &io_write) == 0) {
            printf("مصرف CPU: %" PRIu64 "%%\n", cpu_usage);
            printf("مصرف حافظه: %lu MB\n", mem_usage / (1024 * 1024));
            printf("خواندن I/O: %lu KB\n", io_read / 1024);
            printf("نوشتن I/O: %lu KB\n", io_write / 1024);
        }
    }
    
    printf("محدودیت حافظه: %lu MB\n", config->mem_limit_bytes / (1024 * 1024));
    printf("سهم CPU: %lu\n", config->cpu_shares);
    printf("تخصیص CPU: %s\n", config->cpu_affinity >= 0 ? 
           (char[]){config->cpu_affinity + '0', '\0'} : "تمام هسته‌ها");
    printf("وزن I/O: %lu\n", config->io_weight);
    
    return 0;
}


int container_list(container_manager_t *manager) {
    printf("تعداد کانتینرها: %d\n", manager->container_count);
    printf("--------------------------------------\n");
    printf("%-10s %-20s %-10s %-10s\n", "شناسه", "نام", "وضعیت", "PID");
    printf("--------------------------------------\n");
    
    for (int i = 0; i < manager->container_count; i++) {
        container_config_t *config = &manager->containers[i];
        printf("%-10s %-20s %-10s %-10d\n", 
               config->id, 
               config->name, 
               config->running ? "اجرا" : "متوقف", 
               config->running ? config->container_pid : -1);
    }
    
    return 0;
}


container_config_t* container_find_by_id(container_manager_t *manager, const char *container_id) {
    for (int i = 0; i < manager->container_count; i++) {
        if (strcmp(manager->containers[i].id, container_id) == 0) {
            return &manager->containers[i];
        }
    }
    return NULL;
}


int container_set_memory_limit(container_manager_t *manager, const char *container_id, uint64_t mem_limit_bytes) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    config->mem_limit_bytes = mem_limit_bytes;
    
    
    if (config->running) {
        return cgroup_set_memory_limit(config, mem_limit_bytes);
    }
    
    return 0;
}


int container_set_cpu_shares(container_manager_t *manager, const char *container_id, uint64_t cpu_shares) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    config->cpu_shares = cpu_shares;
    
    
    if (config->running) {
        return cgroup_set_cpu_shares(config, cpu_shares);
    }
    
    return 0;
}


int container_set_cpu_affinity(container_manager_t *manager, const char *container_id, int cpu_id) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    config->cpu_affinity = cpu_id;
    
    
    if (config->running) {
        return cgroup_set_cpu_affinity(config, cpu_id);
    }
    
    return 0;
}


int container_set_io_weight(container_manager_t *manager, const char *container_id, uint64_t io_weight) {
    container_config_t *config = container_find_by_id(manager, container_id);
    if (!config) {
        log_error("کانتینر با شناسه %s پیدا نشد", container_id);
        return -1;
    }
    
    config->io_weight = io_weight;
    
    
    if (config->running) {
        return cgroup_set_io_weight(config, io_weight);
    }
    
    return 0;
}
