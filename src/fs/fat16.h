#ifndef FAT16_H
#define FAT16_H
#include <stdint.h>

void fat16_init();
void fat16_list_files();
// коментарий для души
void fat16_create_file(const char *name, const char *content);
void fat16_write_file(const char *name, const char *content, uint32_t len);
uint32_t fat16_read_file(const char *name, uint8_t *buffer);
#endif
