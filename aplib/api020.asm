; api020.asm
global api_beep

section .text

api_beep: ;void api_beep(int tone);
    mov edx, 20
    mov eax, [esp+4] ; tone
    int 0x40
    ret
