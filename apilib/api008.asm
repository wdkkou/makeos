; api008.asm
global api_initmalloc

section .text

api_initmalloc: ; void api_initmalloc(void);
    push ebx
    mov edx, 8
    mov ebx, [cs:0x0020] ; malloc領域の番地
    mov eax, ebx
    mov eax, 32*1024 ; 32KB
    mov ecx, [cs:0x0000] ; データセグメントの大きさ
    sub ecx, eax
    int 0x40
    pop ebx
    ret
