; api021.asm
global api_fopen

section .text

api_fopen:   ;void api_fopen(char *fname);
    push ebx
    mov edx, 21
    mov ebx, [esp+8]  ; fname
    int 0x40
    pop ebx
    ret
