CC := gcc
CFLAGS := -c -m32 -march=i486 -nostdlib -fno-pic
OBJ_ALL := bootpack.o hankaku.o nasmfunc.o mysprintf.o mystrcmp.o graphic.o dsctbl.o int.o \
				fifo.o mouse.o keyboard.o memory.o sheet.o timer.o mtask.o window.o console.o file.o
# デフォルト動作

all :
	make run

# ファイル生成規則

%.o : %.c Makefile
	$(CC) $(CFLAGS) $< -o $@

%.bin : %.asm Makefile
	nasm $< -o $@ -l $*.lst

nasmfunc.o : nasmfunc.asm Makefile
	nasm -f elf nasmfunc.asm -o nasmfunc.o -l nasmfunc.lst

hankaku.c : hankaku.txt conv_hankaku
	./conv_hankaku

conv_hankaku : conv_hankaku.c
	gcc -o conv_hankaku conv_hankaku.c

bootpack.hrb : $(OBJ_ALL) har.ls Makefile
	gcc -m32 -march=i486 -nostdlib -fno-pic -T har.ls $(OBJ_ALL) -o bootpack.hrb

haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

haribote.img : ipl.bin haribote.sys
	mformat -f 1440 -C -B ipl.bin -i haribote.img ::
	mcopy haribote.sys -i haribote.img ::
# コマンド

asm :
	make -r ipl.bin

img :
	make -r haribote.img

run :
	make img

clean :
	rm *.o *.lst hankaku.c conv_hankaku *.hrb *.sys
