#ifndef MONITOR_H
#define MONITOR_H

#include "container.h"
#include <stdint.h>


int monitor_init();


int monitor_cleanup();


int monitor_container(container_config_t *config);


int monitor_stop_container(container_config_t *config);


int monitor_get_resource_usage(container_config_t *config, uint64_t *cpu_usage, uint64_t *mem_usage, uint64_t *io_read, uint64_t *io_write);


int monitor_namespace_events();


int monitor_cgroup_events();


int monitor_syscall_events();


int monitor_save_logs(const char *container_id);

#endif /* MONITOR_H */