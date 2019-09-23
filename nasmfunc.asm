;nasmfunc.asm
;TAB=4

section .text
    GLOBAL  io_hlt
    GLOBAL  write_mem8

io_hlt:
    HLT
    RET

write_mem8:
    MOV ECX, [ESP+4]
    MOV AL, [ESP+8]
    MOV [ECX], AL
    RET
