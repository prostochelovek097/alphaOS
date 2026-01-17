[BITS 16]
[ORG 0x7c00]
start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    mov ah, 0x02
    mov al, 4
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x80
    mov bx, 0x7E00
    int 0x13
    jc err
    jmp 0x0000:0x7E00
err:
    mov ah, 0x0e
    mov al, 'E'
    int 0x10
    cli
    hlt
times 510-($-$$) db 0
dw 0xAA55
