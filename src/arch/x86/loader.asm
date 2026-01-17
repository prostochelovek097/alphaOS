[BITS 16]
[ORG 0x7E00]
VESA_INFO equ 0x9000
KERNEL_ADDR equ 0x10000
start_loader:
    mov ax, 0x4F01
    mov cx, 0x115
    mov di, VESA_INFO
    int 0x10
    cmp ax, 0x004F
    jne err
    mov ax, 0x4F02
    mov bx, 0x115 | 0x4000
    int 0x10
    cmp ax, 0x004F
    jne err
    mov ah, 0x02
    mov al, 64
    mov ch, 0
    mov cl, 6
    mov dh, 0
    mov dl, 0x80
    mov bx, 0x1000
    mov es, bx
    xor bx, bx
    int 0x13
    jc err
    in al, 0x92
    or al, 2
    out 0x92, al
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pmode
err:
    mov al, '!'
    mov ah, 0x0e
    int 0x10
    cli
    hlt
[BITS 32]
pmode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000
    mov esi, [VESA_INFO + 40]
    push esi
    call KERNEL_ADDR
    hlt
gdt_start: dq 0
gdt_code:  dw 0xFFFF, 0, 0x9A00, 0x00CF
gdt_data:  dw 0xFFFF, 0, 0x9200, 0x00CF
gdt_end:
gdt_desc:  dw gdt_end - gdt_start - 1
           dd gdt_start
