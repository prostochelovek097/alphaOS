#ifndef STRING_H
#define STRING_H
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void *memset(void *ptr, int val, size_t size);
void *memcpy(void *dest, const void *src, size_t size);
int memcmp(const void *ptr1, const void *ptr2, size_t num); // <--- ДОБАВИЛИ
size_t strlen(const char *str);
void strcpy(char *dest, const char *src);
int strcmp(const char *s1, const char *s2);
char toupper(char c);
int strcasecmp(const char *s1, const char *s2);
#endif
