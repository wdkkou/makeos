; api027.asm
global api_getlang

section .text

api_getlang:   ;int api_getlang(void);
    mov edx, 27
    int 0x40
    ret
