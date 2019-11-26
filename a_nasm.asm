global api_putchar
global api_putstr
global api_end
global api_openwin
global api_putstrwin
global api_boxfillwin

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

api_putstrwin: ; void api_putstrwin(int win,int x,int y, int col,int len, char *str);
    push edi
    push esi
    push ebp
    push ebx
    mov edx, 6
    mov ebx, [esp+20] ; win
    mov esi, [esp+24] ; x
    mov edi, [esp+28] ; y
    mov eax, [esp+32] ; col
    mov ecx, [esp+36] ; len
    mov ebp, [esp+40] ; str
    int 0x40
    pop ebx
    pop ebp
    pop esi
    pop edi
    ret

api_boxfillwin: ; void api_boxfillwin(int win,int x0,int y0, int x1,int y1, int col);
    push edi
    push esi
    push ebp
    push ebx
    mov edx, 7
    mov ebx, [esp+20] ; win
    mov eax, [esp+24] ; x0
    mov ecx, [esp+28] ; y0
    mov esi, [esp+32] ; x1
    mov edi, [esp+36] ; y1
    mov ebp, [esp+40] ; col
    int 0x40
    pop ebx
    pop ebp
    pop esi
    pop edi
    ret
