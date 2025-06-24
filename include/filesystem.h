#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "container.h"


int setup_container_rootfs(container_config_t *config);


int cleanup_container_rootfs(container_config_t *config);


int setup_overlayfs(container_config_t *config);


int do_chroot(const char *path);


int prepare_container_directories(container_config_t *config);


int mount_essential_filesystems(container_config_t *config);


int load_container_image(container_config_t *config, const char *image_path);

#endif /* FILESYSTEM_H */