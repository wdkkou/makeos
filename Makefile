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

a_nasm.o : a_nasm.asm Makefile
	nasm -f elf a_nasm.asm -o a_nasm.o -l a_nasm.lst

hankaku.c : hankaku.txt conv_hankaku
	./conv_hankaku

conv_hankaku : conv_hankaku.c
	gcc -o conv_hankaku conv_hankaku.c

bootpack.hrb : $(OBJ_ALL) har.ls Makefile
	gcc -m32 -march=i486 -nostdlib -fno-pic -T har.ls $(OBJ_ALL) -o bootpack.hrb

haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

hello3.bin: hello3.o a_nasm.o api.ls
	gcc -m32 -march=i486 -nostdlib -fno-pic -T api.ls a_nasm.o hello3.o -o hello3.bin

hello4.bin: hello4.o a_nasm.o api.ls
	gcc -m32 -march=i486 -nostdlib -fno-pic -T api.ls a_nasm.o hello4.o -o hello4.bin

# asmファイルから指定のリンカスクリプトの設定にしたアプリを生成する方法
hello5.bin: hello5.asm a_nasm.o api.ls
	nasm -f elf hello5.asm -o hello5.o -l hello5.lst
	gcc -m32 -march=i486 -nostdlib -fno-pic -T api.ls a_nasm.o hello5.o -o hello5.bin

winhello.bin: winhello.o a_nasm.o api.ls
	gcc -m32 -march=i486 -nostdlib -fno-pic -T api.ls a_nasm.o winhello.o -o winhello.bin

# bug1.bin: bug1.o a_nasm.o api.ls
# 	gcc -m32 -march=i486 -nostdlib -fno-pic -T api.ls a_nasm.o bug1.o -o bug1.bin

# bug2.bin: bug2.o a_nasm.o api.ls
# 	gcc -m32 -march=i486 -nostdlib -fno-pic -T api.ls a_nasm.o bug2.o -o bug2.bin

# bug3.bin: bug3.o a_nasm.o api.ls
# 	gcc -m32 -march=i486 -nostdlib -fno-pic -T api.ls a_nasm.o bug3.o -o bug3.bin

# crack1.bin: crack1.o api.ls
# 	gcc -m32 -march=i486 -nostdlib -fno-pic -T api.ls a_nasm.o crack1.o -o crack1.bin

haribote.img : ipl.bin hello.bin hello2.bin hello3.bin hello4.bin hello5.bin winhello.bin haribote.sys Makefile
	mformat -f 1440 -C -B ipl.bin -i haribote.img ::
	mcopy haribote.sys -i haribote.img ::
	mcopy cat.txt -i haribote.img ::
	mcopy hello.bin -i haribote.img ::
	mcopy hello2.bin -i haribote.img ::
	mcopy hello3.bin -i haribote.img ::
	mcopy hello4.bin -i haribote.img ::
	mcopy hello5.bin -i haribote.img ::
	mcopy winhello.bin -i haribote.img ::

# コマンド

asm :
	make -r ipl.bin

img :
	make -r haribote.img

run :
	make img
	qemu-system-i386 -m 32 -fda haribote.img

vdi : haribote.img
	rm ./helloos.vdi -f
	VBoxManage convertfromraw --format VDI haribote.img haribote.vdi

clean :
	rm *.bin *.o *.lst hankaku.c conv_hankaku *.img *.hrb *.sys

debug:
	make img
	qemu-system-i386 -fda haribote.img -gdb tcp::100000 -S
