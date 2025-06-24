#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "../include/container.h"
#include "../include/namespace.h"
#include "../include/cgroup.h"
#include "../include/filesystem.h"
#include "../include/utils.h"


void test_container_manager() {
    printf("تست مدیریت کانتینر...\n");
    
    
    container_manager_t *manager = container_manager_create(10);
    assert(manager != NULL);
    assert(manager->max_containers == 10);
    assert(manager->container_count == 0);
    
    
    container_manager_destroy(manager);
    
    printf("تست مدیریت کانتینر با موفقیت انجام شد\n");
}


void test_container_create() {
    printf("تست ایجاد کانتینر...\n");
    
    
    container_manager_t *manager = container_manager_create(10);
    assert(manager != NULL);
    
    
    char *args[] = {"/bin/echo", "hello", NULL};
    
    
    int result = container_create(manager, "test_container", "/bin/echo", args, 2);
    assert(result == 0);
    assert(manager->container_count == 1);
    
    
    container_config_t *config = &manager->containers[0];
    assert(strcmp(config->name, "test_container") == 0);
    assert(strcmp(config->binary_path, "/bin/echo") == 0);
    assert(config->running == false);
    
    
    container_manager_destroy(manager);
    
    printf("تست ایجاد کانتینر با موفقیت انجام شد\n");
}


void test_container_find() {
    printf("تست جستجوی کانتینر...\n");
    
    
    container_manager_t *manager = container_manager_create(10);
    assert(manager != NULL);
    
    
    char *args[] = {"/bin/echo", "hello", NULL};
    
    
    int result = container_create(manager, "test_container", "/bin/echo", args, 2);
    assert(result == 0);
    
    
    char container_id[64];
    strncpy(container_id, manager->containers[0].id, sizeof(container_id));
    
    
    container_config_t *found = container_find_by_id(manager, container_id);
    assert(found != NULL);
    assert(strcmp(found->name, "test_container") == 0);
    
    
    found = container_find_by_id(manager, "invalid_id");
    assert(found == NULL);
    
    
    container_manager_destroy(manager);
    
    printf("تست جستجوی کانتینر با موفقیت انجام شد\n");
}


int main() {
    printf("شروع آزمون‌های واحد...\n");
    
    test_container_manager();
    test_container_create();
    test_container_find();
    
    printf("تمام آزمون‌ها با موفقیت انجام شدند\n");
    return 0;
}