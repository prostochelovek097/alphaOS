#ifndef PORTS_H
#define PORTS_H
#include <stdint.h>
static inline void outb(uint16_t port, uint8_t val) { asm volatile("outb %0, %1"::"a"(val),"Nd"(port)); }
static inline uint8_t inb(uint16_t port) { uint8_t r; asm volatile("inb %1, %0":"=a"(r):"Nd"(port)); return r; }
static inline void insw(uint16_t port, void* addr, int cnt) { asm volatile("rep insw":"+D"(addr),"+c"(cnt):"d"(port):"memory"); }
static inline void outsw(uint16_t port, void* addr, int cnt) { asm volatile("rep outsw":"+S"(addr),"+c"(cnt):"d"(port):"memory"); }
#endif
