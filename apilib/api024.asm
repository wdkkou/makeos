; api024.asm
global api_fsize

section .text

api_fsize:   ;int api_fsize(int handle,int mode);
    mov edx, 24
    mov eax, [esp+4]  ; fhandle
    mov ecx, [esp+8]  ; fhandle
    int 0x40
    ret
