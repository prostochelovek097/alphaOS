#ifndef VESA_H
#define VESA_H
#include <stdint.h>
void vesa_init(uint32_t buffer_addr);
void vesa_clear(uint32_t color);
void vesa_put_pixel(int x, int y, uint32_t color);
void vesa_print(const char* str);
void vesa_print_color(const char* str, uint32_t color);
#endif
