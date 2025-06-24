#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdbool.h>


int generate_unique_id(char *buffer, size_t buffer_size);


bool directory_exists(const char *path);


int create_directory(const char *path, int mode);


int remove_directory(const char *path);


void log_message(const char *format, ...);


void log_error(const char *format, ...);


bool has_root_privileges();


int copy_file(const char *source, const char *destination);


int remove_file(const char *path);

#endif /* UTILS_H */