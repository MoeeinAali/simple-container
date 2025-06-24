#ifndef IPC_H
#define IPC_H

#include "container.h"


int ipc_setup();


int ipc_cleanup();


int ipc_create_channel(container_config_t *config, const char *channel_name);


int ipc_connect_containers(const char *container_id1, const char *container_id2, const char *channel_name);


int ipc_send_message(const char *channel_name, const void *data, size_t data_size);


int ipc_receive_message(const char *channel_name, void *buffer, size_t buffer_size);

#endif /* IPC_H */