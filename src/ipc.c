#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include "../include/ipc.h"
#include "../include/utils.h"


typedef struct {
    char name[64];
    int type;        
    int id;          
    key_t key;       
} ipc_channel_t;


#define MAX_IPC_CHANNELS 32


static ipc_channel_t channels[MAX_IPC_CHANNELS];
static int channel_count = 0;


int ipc_setup() {
    
    memset(channels, 0, sizeof(channels));
    channel_count = 0;
    
    log_message("سیستم IPC راه‌اندازی شد");
    return 0;
}


int ipc_cleanup() {
    
    for (int i = 0; i < channel_count; i++) {
        if (channels[i].type == 1) {
            
            shmctl(channels[i].id, IPC_RMID, NULL);
        } else if (channels[i].type == 2) {
            
            semctl(channels[i].id, 0, IPC_RMID);
        } else if (channels[i].type == 3) {
            
            msgctl(channels[i].id, IPC_RMID, NULL);
        }
    }
    
    
    memset(channels, 0, sizeof(channels));
    channel_count = 0;
    
    log_message("منابع IPC پاک‌سازی شدند");
    return 0;
}


static ipc_channel_t* find_channel(const char *channel_name) {
    for (int i = 0; i < channel_count; i++) {
        if (strcmp(channels[i].name, channel_name) == 0) {
            return &channels[i];
        }
    }
    return NULL;
}


int ipc_create_channel(container_config_t *config, const char *channel_name) {
    if (channel_count >= MAX_IPC_CHANNELS) {
        log_error("حداکثر تعداد کانال‌های IPC ایجاد شده است");
        return -1;
    }
    
    
    if (find_channel(channel_name) != NULL) {
        log_error("کانال IPC با نام %s قبلاً ایجاد شده است", channel_name);
        return -1;
    }
    
    
    key_t key = ftok("/var/lib/simplecontainer", channel_count + 1);
    if (key == -1) {
        log_error("خطا در ایجاد کلید IPC");
        return -1;
    }
    
    
    int shm_id = shmget(key, 4096, IPC_CREAT | 0666);
    if (shm_id == -1) {
        log_error("خطا در ایجاد حافظه مشترک");
        return -1;
    }
    
    
    ipc_channel_t *channel = &channels[channel_count++];
    strncpy(channel->name, channel_name, sizeof(channel->name) - 1);
    channel->type = 1;  
    channel->id = shm_id;
    channel->key = key;
    
    log_message("کانال IPC %s برای کانتینر %s ایجاد شد", channel_name, config->id);
    return 0;
}


int ipc_connect_containers(const char *container_id1, const char *container_id2, const char *channel_name) {
    
    ipc_channel_t *channel = find_channel(channel_name);
    if (channel == NULL) {
        log_error("کانال IPC با نام %s پیدا نشد", channel_name);
        return -1;
    }
    
    log_message("کانتینرهای %s و %s از طریق کانال %s به هم متصل شدند", 
                container_id1, container_id2, channel_name);
    return 0;
}


int ipc_send_message(const char *channel_name, const void *data, size_t data_size) {
    
    ipc_channel_t *channel = find_channel(channel_name);
    if (channel == NULL) {
        log_error("کانال IPC با نام %s پیدا نشد", channel_name);
        return -1;
    }
    
    
    if (channel->type != 1) {
        log_error("کانال %s از نوع حافظه مشترک نیست", channel_name);
        return -1;
    }
    
    
    void *shm_addr = shmat(channel->id, NULL, 0);
    if (shm_addr == (void *) -1) {
        log_error("خطا در اتصال به حافظه مشترک");
        return -1;
    }
    
    
    if (data_size > 4096) {
        data_size = 4096;  
    }
    
    
    *((uint32_t *)shm_addr) = data_size;
    memcpy((char *)shm_addr + 4, data, data_size);
    
    
    shmdt(shm_addr);
    
    log_message("%lu بایت داده از طریق کانال %s ارسال شد", data_size, channel_name);
    return 0;
}


int ipc_receive_message(const char *channel_name, void *buffer, size_t buffer_size) {
    
    ipc_channel_t *channel = find_channel(channel_name);
    if (channel == NULL) {
        log_error("کانال IPC با نام %s پیدا نشد", channel_name);
        return -1;
    }
    
    
    if (channel->type != 1) {
        log_error("کانال %s از نوع حافظه مشترک نیست", channel_name);
        return -1;
    }
    
    
    void *shm_addr = shmat(channel->id, NULL, SHM_RDONLY);
    if (shm_addr == (void *) -1) {
        log_error("خطا در اتصال به حافظه مشترک");
        return -1;
    }
    
    
    uint32_t data_size = *((uint32_t *)shm_addr);
    
    
    if (data_size > buffer_size) {
        log_error("بافر کوچکتر از داده است (%u > %lu)", data_size, buffer_size);
        shmdt(shm_addr);
        return -1;
    }
    
    
    memcpy(buffer, (char *)shm_addr + 4, data_size);
    
    
    shmdt(shm_addr);
    
    log_message("%u بایت داده از طریق کانال %s دریافت شد", data_size, channel_name);
    return data_size;
}