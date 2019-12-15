; api005.asm
global api_openwin

section .text

api_openwin: ; int api_openwin(char *buf,int xsize,int ysize,int col_inv,char *title);
    push edi
    push esi
    push ebx
    mov edx, 5
    mov ebx, [esp+16] ; buf
    mov esi, [esp+20] ; xsize
    mov edi, [esp+24] ; ysize
    mov eax, [esp+28] ; col_inv
    mov ecx, [esp+32] ; title
    int 0x40
    pop ebx
    pop esi
    pop edi
    ret
