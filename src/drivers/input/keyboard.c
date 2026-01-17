#include <drivers/input/keyboard.h>
#include <arch/x86/ports.h>
static char map[128]={0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '};
char keyboard_get_char() { if(inb(0x64)&1){uint8_t s=inb(0x60); if(!(s&0x80)) return map[s];} return 0; }
