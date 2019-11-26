global api_putchar
global api_putstr
global api_end
global api_openwin

section .text

api_putchar: ; void api_putchar(int c);
    mov edx, 1
    mov al, [esp+4]
    int 0x40
    ret

api_putstr: ; void api_putstr(char *s);
    push ebx
    mov edx, 2
    mov ebx, [esp+8]
    int 0x40
    pop ebx
    ret

api_end: ; void api_end(void);
    mov edx, 4
    int 0x40

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
