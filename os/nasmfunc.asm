;nasmfunc.asm
;TAB=4

    GLOBAL  io_hlt, io_cli, io_sti, io_stihlt
    GLOBAL  io_in8, io_in16, io_in32
    GLOBAL  io_out8, io_out16, io_out32
    GLOBAL  io_load_eflags, io_store_eflags
    GLOBAL  load_gdtr, load_idtr
    GLOBAL  load_cr0, store_cr0
    GLOBAL  load_tr
    GLOBAL  asm_inthandler21, asm_inthandler2c, asm_inthandler20,asm_inthandler0d, asm_inthandler0c
    GLOBAL  asm_end_app
    GLOBAL  memtest_sub
    GLOBAL  farjmp, farcall
    GLOBAL  asm_bin_api, start_app
    EXTERN  inthandler21, inthandler2c, inthandler20 , inthandler0d, inthandler0c
    EXTERN  bin_api

bits 32
section .text

io_hlt:
    HLT
    RET

io_cli:     ; void io_cli(void);
        cli
        ret

io_sti:     ; void io_sti(void);
        sti
        ret

io_stihlt:      ; void io_stihlt(void)
        sti
        hlt
        ret

io_in8:     ; int io_in8(int port);
        mov     edx, [esp + 4]      ; port
        mov     eax, 0
        in      al, dx              ; 8
        ret

io_in16:        ; int io_in16(int port);
        mov     edx, [esp + 4]      ; port
        mov     eax, 0
        in      ax, dx              ; 16
        ret

in_in32:        ; int io_in16(int port);
        mov     edx, [esp + 4]      ; port
        in      eax, dx             ; 32
        ret

io_out8:        ; void io_to_in_out8;
        mov     edx, [esp + 4]      ; port
        mov     al, [esp + 8]       ; data
        out     dx, al              ; 8
        ret

io_out16:       ; void io_to_in_out16;
        mov     edx, [esp + 4]      ; port
        mov     eax, [esp + 8]      ; data
        out     dx, ax              ; 16
        ret

io_out32:       ; void io_to_in_out32;
        mov     edx, [esp + 4]      ; port
        mov     eax, [esp + 8]      ; data
        out     dx, eax             ; 32
        ret

io_load_eflags :     ; int io_load_eflags(void)
        pushfd      ; push eflags double-word
        pop     eax
        ret

io_store_eflags :        ; void io_store_eflags(int eflags)
        mov     eax, [esp + 4]
        push    eax
        popfd       ; pup eflags double-word
        ret

load_gdtr :
        mov ax, [esp + 4]
        mov [esp + 6], ax
        lgdt [esp + 6]
        ret

load_idtr :
        mov ax, [esp + 4]
        mov [esp + 6], ax
        lidt [esp + 6]
        ret

load_cr0:
        mov eax, cr0
        ret

store_cr0:
        mov eax, [esp+4]
        mov cr0, eax
        ret

load_tr:
        ltr [esp+4]
        ret

asm_inthandler20:
        push es
        push ds
        pushad
        mov eax, esp
        push eax
        mov ax, ss
        mov ds, ax
        mov es, ax
        call inthandler20
        pop eax
        popad
        pop ds
        pop es
        iretd

asm_inthandler21:
        push es
        push ds
        pushad
        mov eax, esp
        push eax
        mov ax, ss
        mov ds, ax
        mov es, ax
        call inthandler21
        pop eax
        popad
        pop ds
        pop es
        iretd

asm_inthandler2c:
        push es
        push ds
        pushad
        mov eax, esp
        push eax
        mov ax, ss
        mov ds, ax
        mov es, ax
        call inthandler2c
        pop eax
        popad
        pop ds
        pop es
        iretd

asm_inthandler0c:
        sti
        push es
        push ds
        pushad
        mov eax, esp
        push eax
        mov ax, ss
        mov ds, ax
        mov es, ax
        call inthandler0c
        cmp eax, 0
        jne asm_end_app
        pop eax
        popad
        pop ds
        pop es
        add esp, 4
        iretd

asm_inthandler0d:
        sti
        push es
        push ds
        pushad
        mov eax, esp
        push eax
        mov ax, ss
        mov ds, ax
        mov es, ax
        call inthandler0d
        cmp eax, 0
        jne asm_end_app
        pop eax
        popad
        pop ds
        pop es
        add esp, 4
        iretd

memtest_sub:
        push edi
        push esi
        push ebx
        mov esi, 0xaa55aa55
        mov edi, 0x55aa55aa
        mov eax, [esp+12+4] ;i = start
mts_loop:
        mov ebx, eax
        add ebx,0xffc
        mov edx, [ebx]
        mov [ebx], esi
        xor dword [ebx], 0xffffffff
        cmp edi, [ebx]
        jne mts_fin
        xor dword [ebx], 0xffffffff
        cmp esi, [ebx]
        jne mts_fin
        mov [ebx], edx
        add eax, 0x1000
        cmp eax, [esp+12+8]
        jbe mts_loop
        pop ebx
        pop esi
        pop edi
        ret
mts_fin:
        mov [ebx], edx
        pop ebx
        pop esi
        pop edi
        ret

farjmp:
        jmp far [esp+4]
        ret

farcall:
        call far [esp+4]
        ret

asm_bin_api:
        sti
        push ds
        push es
        pushad ; 保存のためのpush
        pushad ; bin_apiに渡すためのpush
        mov ax, ss
        mov ds, ax
        mov es, ax
        call bin_api
        cmp eax, 0
        jne asm_end_app
        add esp, 32
        popad
        pop es
        pop ds
        iretd
asm_end_app:
; eaxはtss.esp0の番地
        mov esp, [eax]
        mov dword [eax+4],0
        popad
        ret ; cmd_appへ戻る
start_app: ; void start_app(int eip,int cs,int esp,int ds, int *tss_esp0);
        pushad
        mov eax, [esp+36]
        mov ecx, [esp+40]
        mov edx, [esp+44]
        mov ebx, [esp+48]
        mov ebp, [esp+52]
        mov [ebp], esp
        mov [ebp+4], ss
        mov es, bx
        mov ds, bx
        mov fs, bx
        mov gs, bx
; 以下　retfでアプリに行くためのスタック調整
        or ecx, 3
        or ebx, 3
        push ebx
        push edx
        push ecx
        push eax
        retf
; アプリが終了してもここには来ない
