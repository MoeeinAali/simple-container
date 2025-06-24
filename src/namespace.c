#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../include/namespace.h"
#include "../include/utils.h"


int setup_namespaces(container_config_t *config) {
    
    if (setup_uts_namespace(config->name) != 0) {
        log_error("خطا در تنظیم UTS namespace");
        return -1;
    }
    
    
    if (setup_mount_namespace() != 0) {
        log_error("خطا در تنظیم mount namespace");
        return -1;
    }
    
    
    if (setup_pid_namespace() != 0) {
        log_error("خطا در تنظیم PID namespace");
        return -1;
    }
    
    
    if (setup_user_namespace() != 0) {
        log_error("خطا در تنظیم user namespace");
        return -1;
    }
    
    
    if (setup_network_namespace() != 0) {
        log_error("خطا در تنظیم network namespace");
        return -1;
    }
    
    
    if (setup_ipc_namespace() != 0) {
        log_error("خطا در تنظیم IPC namespace");
        return -1;
    }
    
    return 0;
}


int setup_user_namespace() {
    
    int uid_map_fd = open("/proc/self/uid_map", O_WRONLY);
    if (uid_map_fd == -1) {
        log_error("خطا در باز کردن /proc/self/uid_map");
        return -1;
    }
    
    char uid_map[100];
    snprintf(uid_map, sizeof(uid_map), "0 %d 1", getuid());
    
    ssize_t uid_bytes = write(uid_map_fd, uid_map, strlen(uid_map));
    if (uid_bytes != (ssize_t)strlen(uid_map)) {
        log_error("خطا در نوشتن به uid_map");
        close(uid_map_fd);
        return -1;
    }
    close(uid_map_fd);
    
    
    int setgroups_fd = open("/proc/self/setgroups", O_WRONLY);
    if (setgroups_fd != -1) {
        const char *deny = "deny";
        write(setgroups_fd, deny, strlen(deny));
        close(setgroups_fd);
    }
    
    
    int gid_map_fd = open("/proc/self/gid_map", O_WRONLY);
    if (gid_map_fd == -1) {
        log_error("خطا در باز کردن /proc/self/gid_map");
        return -1;
    }
    
    char gid_map[100];
    snprintf(gid_map, sizeof(gid_map), "0 %d 1", getgid());
    
    ssize_t gid_bytes = write(gid_map_fd, gid_map, strlen(gid_map));
    if (gid_bytes != (ssize_t)strlen(gid_map)) {
        log_error("خطا در نوشتن به gid_map");
        close(gid_map_fd);
        return -1;
    }
    close(gid_map_fd);
    
    return 0;
}


int setup_pid_namespace() {
    
    
    if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
        log_error("خطا در نصب /proc");
        return -1;
    }
    return 0;
}


int setup_mount_namespace() {
    
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) {
        log_error("خطا در تنظیم MS_PRIVATE برای /");
        return -1;
    }
    return 0;
}


int setup_uts_namespace(const char *hostname) {
    
    if (sethostname(hostname, strlen(hostname)) != 0) {
        log_error("خطا در تنظیم hostname");
        return -1;
    }
    return 0;
}


int setup_network_namespace() {
    
    system("ip link set lo up");
    return 0;
}


int setup_ipc_namespace() {
    
    return 0;
}


int join_namespace(pid_t pid, int nstype) {
    char ns_path[256];
    
    
    const char *ns_name = get_namespace_path(nstype);
    if (ns_name == NULL) {
        log_error("نوع namespace نامعتبر");
        return -1;
    }
    
    snprintf(ns_path, sizeof(ns_path), "/proc/%d/ns/%s", pid, ns_name);
    
    
    int fd = open(ns_path, O_RDONLY);
    if (fd == -1) {
        log_error("خطا در باز کردن namespace: %s", ns_path);
        return -1;
    }
    
    
    if (setns(fd, nstype) == -1) {
        log_error("خطا در اتصال به namespace");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}


const char* get_namespace_path(int nstype) {
    switch (nstype) {
        case CLONE_NEWPID:
            return "pid";
        
        case CLONE_NEWNS:
            return "mnt";
        
        case CLONE_NEWUTS:
            return "uts";
        
        case CLONE_NEWUSER:
            return "user";
        
        case CLONE_NEWNET:
            return "net";
        
        case CLONE_NEWIPC:
            return "ipc";
        
        default:
            return NULL;
    }
}


bool namespace_exists(pid_t pid, int nstype) {
    char ns_path[256];
    
    const char *ns_name = get_namespace_path(nstype);
    if (ns_name == NULL) {
        return false;
    }
    
    snprintf(ns_path, sizeof(ns_path), "/proc/%d/ns/%s", pid, ns_name);
    
    
    if (access(ns_path, F_OK) == 0) {
        return true;
    }
    
    return false;
}