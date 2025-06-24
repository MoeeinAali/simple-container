#ifndef CONTAINER_H
#define CONTAINER_H

#include <stdint.h>
#include <stdbool.h>


typedef struct {
    char id[64];                
    char name[256];             
    char rootfs[512];           
    char overlay_workdir[512];  
    char binary_path[512];      
    char **args;                
    int argc;                   
    
    
    uint64_t mem_limit_bytes;   
    uint64_t cpu_shares;        
    int cpu_affinity;           
    uint64_t io_weight;         
    
    pid_t container_pid;        
    bool running;               
    char cgroup_path[512];      
} container_config_t;


typedef struct {
    container_config_t *containers;
    int max_containers;
    int container_count;
} container_manager_t;


container_manager_t* container_manager_create(int max_containers);
void container_manager_destroy(container_manager_t *manager);


int container_create(container_manager_t *manager, const char *name, const char *binary_path, char **args, int argc);
int container_start(container_manager_t *manager, const char *container_id);
int container_stop(container_manager_t *manager, const char *container_id);
int container_status(container_manager_t *manager, const char *container_id);
int container_list(container_manager_t *manager);


int container_set_memory_limit(container_manager_t *manager, const char *container_id, uint64_t mem_limit_bytes);
int container_set_cpu_shares(container_manager_t *manager, const char *container_id, uint64_t cpu_shares);
int container_set_cpu_affinity(container_manager_t *manager, const char *container_id, int cpu_id);
int container_set_io_weight(container_manager_t *manager, const char *container_id, uint64_t io_weight);


container_config_t* container_find_by_id(container_manager_t *manager, const char *container_id);
int container_setup_environment(container_config_t *config);
int container_cleanup_environment(container_config_t *config);

#endif /* CONTAINER_H */