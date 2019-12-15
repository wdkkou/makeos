; api003.asm
global api_putstr_len

section .text

api_putstr_len: ; void api_putstr_len(char *s, int l);
    push ebx
    mov edx, 3
    mov ebx, [esp+8] ; s
    mov ecx, [esp+12] ; l
    int 0x40
    pop ebx
    ret
