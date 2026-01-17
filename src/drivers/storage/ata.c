#include <drivers/storage/ata.h>
#include <arch/x86/ports.h>
void wait(){while((inb(0x1F7)&0x88)!=0x08);}
void ata_read_sector(uint32_t lba, uint8_t* buf) {
 outb(0x1F6,0xE0|((lba>>24)&0xF)); outb(0x1F2,1); outb(0x1F3,(uint8_t)lba);
 outb(0x1F4,(uint8_t)(lba>>8)); outb(0x1F5,(uint8_t)(lba>>16)); outb(0x1F7,0x20);
 wait(); insw(0x1F0,buf,256);
}
void ata_write_sector(uint32_t lba, uint8_t* buf) {
 outb(0x1F6,0xE0|((lba>>24)&0xF)); outb(0x1F2,1); outb(0x1F3,(uint8_t)lba);
 outb(0x1F4,(uint8_t)(lba>>8)); outb(0x1F5,(uint8_t)(lba>>16)); outb(0x1F7,0x30);
 wait(); outsw(0x1F0,buf,256); outb(0x1F7,0xE7); while(inb(0x1F7)&0x80);
}
