;nasmfunc.asm
;TAB=4

    GLOBAL  io_hlt, io_cli, io_sti, io_stihlt
    GLOBAL  io_in8, io_in16, io_in32
    GLOBAL  io_out8, io_out16, io_out32
    GLOBAL  io_load_eflags, io_store_eflags
    GLOBAL  load_gdtr, load_idtr
    GLOBAL  load_cr0, store_cr0
    GLOBAL  load_tr
    GLOBAL  asm_inthandler21, asm_inthandler2c, asm_inthandler27,asm_inthandler20,asm_inthandler0d
    GLOBAL  memtest_sub
    GLOBAL  farjmp, farcall
    GLOBAL  asm_bin_api, start_app
    EXTERN  inthandler21, inthandler2c, inthandler27, inthandler20 , inthandler0d
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
        mov ax, ss
        cmp ax, 1*8
        jne .from_app
        ;osが動いている時に割り込まれたのでほぼ今までどおり
        mov eax,esp
        push ss
        push eax
        mov ax, ss
        mov ds, ax
        mov es,ax
        call inthandler20
        add esp, 8
        popad
        pop ds
        pop es
        iretd
.from_app
; アプリが動いている時に割り込まれた
        mov eax, 1*8
        mov ds, ax
        mov ecx, [0xfe4]
        add ecx, -8
        mov [ecx+4], ss
        mov [ecx], esp
        mov ss, ax
        mov es, ax
        mov esp, ecx
        call inthandler20
        pop ecx
        pop eax
        mov ss, ax
        mov esp , ecx
        popad
        pop ds
        pop es
        iretd

asm_inthandler21:
        push es
        push ds
        pushad
        mov ax, ss
        cmp ax, 1*8
        jne .from_app
        ;osが動いている時に割り込まれたのでほぼ今までどおり
        mov eax,esp
        push ss
        push eax
        mov ax, ss
        mov ds, ax
        mov es,ax
        call inthandler21
        add esp, 8
        popad
        pop ds
        pop es
        iretd
.from_app
; アプリが動いている時に割り込まれた
        mov eax, 1*8
        mov ds, ax
        mov ecx, [0xfe4]
        add ecx, -8
        mov [ecx+4], ss
        mov [ecx], esp
        mov ss, ax
        mov es, ax
        mov esp, ecx
        call inthandler21
        pop ecx
        pop eax
        mov ss, ax
        mov esp , ecx
        popad
        pop ds
        pop es
        iretd

asm_inthandler27:
        push es
        push ds
        pushad
        mov ax, ss
        cmp ax, 1*8
        jne .from_app
        ;osが動いている時に割り込まれたのでほぼ今までどおり
        mov eax,esp
        push ss
        push eax
        mov ax, ss
        mov ds, ax
        mov es,ax
        call inthandler27
        add esp, 8
        popad
        pop ds
        pop es
        iretd
.from_app
; アプリが動いている時に割り込まれた
        mov eax, 1*8
        mov ds, ax
        mov ecx, [0xfe4]
        add ecx, -8
        mov [ecx+4], ss
        mov [ecx], esp
        mov ss, ax
        mov es, ax
        mov esp, ecx
        call inthandler27
        pop ecx
        pop eax
        mov ss, ax
        mov esp , ecx
        popad
        pop ds
        pop es
        iretd

asm_inthandler2c:
        push es
        push ds
        pushad
        mov ax, ss
        cmp ax, 1*8
        jne .from_app
        ;osが動いている時に割り込まれたのでほぼ今までどおり
        mov eax,esp
        push ss
        push eax
        mov ax, ss
        mov ds, ax
        mov es,ax
        call inthandler2c
        add esp, 8
        popad
        pop ds
        pop es
        iretd
.from_app
; アプリが動いている時に割り込まれた
        mov eax, 1*8
        mov ds, ax
        mov ecx, [0xfe4]
        add ecx, -8
        mov [ecx+4], ss
        mov [ecx], esp
        mov ss, ax
        mov es, ax
        mov esp, ecx
        call inthandler2c
        pop ecx
        pop eax
        mov ss, ax
        mov esp , ecx
        popad
        pop ds
        pop es
        iretd

asm_inthandler0d:
        sti
        push es
        push ds
        pushad
        mov ax, ss
        cmp ax, 1*8
        jne .from_app
        ;osが動いている時に割り込まれたのでほぼ今までどおり
        mov eax,esp
        push ss
        push eax
        mov ax, ss
        mov ds, ax
        mov es,ax
        call inthandler20
        add esp, 8
        popad
        pop ds
        pop es
        iretd
.from_app
; アプリが動いている時に割り込まれた
        cli
        mov eax, 1*8
        mov ds, ax
        mov ecx, [0xfe4]
        add ecx, -8
        mov [ecx+4], ss
        mov [ecx], esp
        mov ss, ax
        mov es, ax
        mov esp, ecx
        sti
        call inthandler0d
        cli
        cmp eax, 0
        jne .kill
        pop ecx
        pop eax
        mov ss, ax
        mov esp , ecx
        popad
        pop ds
        pop es
        iretd
.kill:
; アプリを異常終了させる
        mov eax, 1*8
        mov es, ax
        mov ss, ax
        mov ds, ax
        mov fs, ax
        mov gs, ax
        mov esp, [0xfe4] ; start_appの時のespに無理やり戻す
        popad ;保存しておいたレジスタを回復
        ret

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
        push ds
        push es
        pushad ; 保存のためのpush
        mov eax, 1*8
        mov ds, ax
        mov ecx, [0xfe4]
        add ecx,-40
        mov [ecx+32], esp
        mov [ecx+36], ss
        ; pushadした値をシステムのスタックにコピー
        mov edx, [esp]
        mov ebx, [esp+4]
        mov [ecx] ,edx
        mov [ecx+4] ,ebx
        mov edx, [esp+8]
        mov ebx, [esp+12]
        mov [ecx+8] ,edx
        mov [ecx+12] ,ebx
        mov edx, [esp+16]
        mov ebx, [esp+20]
        mov [ecx+16] ,edx
        mov [ecx+20] ,ebx
        mov edx, [esp+24]
        mov ebx, [esp+28]
        mov [ecx+24] ,edx
        mov [ecx+28] ,ebx

        mov es, ax
        mov ss, ax
        mov esp, ecx
        sti ; 割り込み許可

        call bin_api

        mov ecx, [esp+32]
        mov eax, [esp+36]
        cli
        mov ss, ax
        mov esp, ecx
        popad
        pop es
        pop ds
        iretd

start_app: ; void start_app(int eip,int cs,int esp,int ds);
        pushad
        mov eax, [esp+36]
        mov ecx, [esp+40]
        mov edx, [esp+44]
        mov ebx, [esp+48]
        mov [0xfe4], esp
        cli
        mov es, bx
        mov ss, bx
        mov ds, bx
        mov fs, bx
        mov gs, bx
        mov esp, edx
        sti
        push ecx ; far-callのためにpush(cs)
        push eax ; far-callのためにpush(eip)
        call far [esp] ; アプリを呼び出す

; アプリが終了するとここに帰ってくる

        mov eax, 1*8
        cli
        mov es, ax
        mov ss, ax
        mov ds, ax
        mov fs, ax
        mov gs, ax
        mov esp, [0xfe4]
        sti
        popad
        ret

