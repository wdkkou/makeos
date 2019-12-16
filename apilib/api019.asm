; api019.asm
global api_freetimer

section .text

api_freetimer:
    push ebx
    mov edx, 19
    mov ebx,[esp+8]
    int 0x40
    pop ebx
    ret
