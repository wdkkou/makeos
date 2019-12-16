; api001.asm
global api_putchar

section .text

api_putchar: ; void api_putchar(int c);
    mov edx, 1
    mov al, [esp+4]
    int 0x40
    ret
