; api013.asm
global api_linewin

section .text

api_linewin: ; void api_linewin(int win,int x0, int y0,int x1,int y1,int col);
    push edi
    push esi
    push ebp
    push ebx
    mov edx, 13
    mov ebx, [esp+20]
    mov eax, [esp+24]
    mov ecx, [esp+28]
    mov esi, [esp+32]
    mov edi, [esp+36]
    mov ebp, [esp+40]
    int 0x40
    pop ebx
    pop ebp
    pop esi
    pop edi
    ret
