#ifndef NAMESPACE_H
#define NAMESPACE_H

#include <sys/types.h>
#include <stdbool.h>
#include "container.h"


int setup_namespaces(container_config_t *config);


int setup_user_namespace();
int setup_pid_namespace();
int setup_mount_namespace();
int setup_uts_namespace(const char *hostname);
int setup_network_namespace();
int setup_ipc_namespace();


int join_namespace(pid_t pid, int nstype);


const char* get_namespace_path(int nstype);


bool namespace_exists(pid_t pid, int nstype);

#endif 