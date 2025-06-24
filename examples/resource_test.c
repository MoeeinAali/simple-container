#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>

void test_memory_limit() {
    printf("تلاش برای تخصیص 100MB حافظه...\n");
    
        void *ptr = malloc(100 * 1024 * 1024);
    
    if (ptr == NULL) {
        printf("خطای تخصیص حافظه: محدودیت حافظه اعمال شد\n");
        exit(1);
    }
    
        memset(ptr, 1, 100 * 1024 * 1024);
    
    printf("تخصیص حافظه موفقیت‌آمیز بود\n");
    free(ptr);
}

void test_cpu_affinity() {
    printf("در حال اجرای محاسبات روی CPU...\n");
    
        for (int i = 0; i < 1000000; i++) {
        sqrt((double)i);
    }
    
        cpu_set_t mask;
    CPU_ZERO(&mask);
    if (sched_getaffinity(0, sizeof(mask), &mask) == -1) {
        perror("sched_getaffinity");
        exit(1);
    }
    
        for (int i = 0; i < CPU_SETSIZE; i++) {
        if (CPU_ISSET(i, &mask)) {
            printf("CPU ID فعال: %d\n", i);
            break;
        }
    }
}

void test_io_limit() {
    printf("در حال انجام عملیات I/O سنگین...\n");
    
        const char *filename = "/tmp/io_test";
    const size_t file_size = 50 * 1024 * 1024;      
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open for writing");
        exit(1);
    }
    
    char buffer[4096];
    memset(buffer, 'A', sizeof(buffer));
    
        struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    
        for (size_t i = 0; i < file_size; i += sizeof(buffer)) {
        if (write(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {
            perror("write");
            close(fd);
            exit(1);
        }
    }
    
    fsync(fd);
    close(fd);
    
        gettimeofday(&end_time, NULL);
    
        double elapsed = (end_time.tv_sec - start_time.tv_sec) +
                     (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
    
    printf("I/O انجام شد در %.2f ثانیه\n", elapsed);
    
        unlink(filename);
}

int main(int argc, char **argv) {
        if (argc < 2) {
        printf("استفاده: %s [--memory-test|--cpu-test|--io-test]\n", argv[0]);
        return 1;
    }
    
        if (strcmp(argv[1], "--memory-test") == 0) {
        test_memory_limit();
    } else if (strcmp(argv[1], "--cpu-test") == 0) {
        test_cpu_affinity();
    } else if (strcmp(argv[1], "--io-test") == 0) {
        test_io_limit();
    } else {
        printf("آرگومان نامعتبر: %s\n", argv[1]);
        printf("گزینه‌های معتبر: --memory-test, --cpu-test, --io-test\n");
        return 1;
    }
    
    return 0;
}