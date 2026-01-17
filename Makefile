CC = gcc
LD = ld
ASM = nasm

CFLAGS = -m32 -ffreestanding -O2 -Wall -Wextra -fno-pie -fno-stack-protector -Isrc
LDFLAGS = -m elf_i386 -T linker.ld --oformat binary

C_SOURCES = $(wildcard src/kernel/*.c src/drivers/*/*.c src/fs/*.c src/lib/*.c)
OBJ = $(C_SOURCES:.c=.o)

all: os.img

os.img: boot.bin loader.bin kernel.bin
	dd if=/dev/zero of=os.img bs=1M count=20
	mkfs.fat -F 16 -R 200 -n "ALPHAOS" os.img
	touch logo/README.TXT
	mcopy -i os.img logo/* ::/
	dd if=boot.bin of=os.img conv=notrunc bs=512 count=1
	dd if=loader.bin of=os.img seek=1 conv=notrunc bs=512
	dd if=kernel.bin of=os.img seek=5 conv=notrunc bs=512

boot.bin: src/arch/x86/boot.asm
	$(ASM) -f bin $< -o $@

loader.bin: src/arch/x86/loader.asm
	$(ASM) -f bin $< -o $@

kernel.bin: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) *.bin *.img qemu.log

run: os.img
	qemu-system-i386 -hda os.img -vga std -m 128 -serial file:qemu.log -d guest_errors \
	-machine pcspk-audiodev=snd0 -audiodev pa,id=snd0
