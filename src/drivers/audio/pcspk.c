#include <drivers/audio/pcspk.h>
#include <arch/x86/ports.h>
static void delay(uint32_t c){for(volatile uint32_t i=0;i<c*1000;i++);}
void beep(uint32_t f, uint32_t ms) {
 if(f>0){uint32_t d=1193180/f; outb(0x43,0xb6); outb(0x42,d); outb(0x42,d>>8); outb(0x61,inb(0x61)|3);}
 else {outb(0x61,inb(0x61)&0xFC);}
 delay(ms); outb(0x61,inb(0x61)&0xFC);
}
void play_imperial_march(){beep(440,500);beep(440,500);beep(440,500);beep(349,350);beep(523,150);beep(440,500);beep(349,350);beep(523,150);beep(440,1000);}
