CC := gcc
CFLAGS := -c -m32 -march=i486 -nostdlib
OBJ_ALL := bootpack.o hankaku.o nasmfunc.o mysprintf.o graphic.o dsctbl.o
# デフォルト動作

all :
	make run

# ファイル生成規則

%.o : %.c Makefile
	$(CC) $(CFLAGS) $< -o $@

%.bin : %.asm Makefile
	nasm $< -o $@

nasmfunc.o : nasmfunc.asm Makefile
	nasm -f elf nasmfunc.asm -o nasmfunc.o -l nasmfunc.lst

hankaku.c : hankaku.txt conv_hankaku
	./conv_hankaku

conv_hankaku : conv_hankaku.c
	gcc -o conv_hankaku conv_hankaku.c

bootpack.hrb : $(OBJ_ALL) har.ls Makefile
	gcc -m32 -march=i486 -nostdlib -nostdinc -T har.ls $(OBJ_ALL) -o bootpack.hrb

haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

haribote.img : ipl.bin haribote.sys Makefile
	mformat -f 1440 -C -B ipl.bin -i haribote.img ::
	mcopy haribote.sys -i haribote.img ::


# コマンド

asm :
	make -r ipl.bin

img :
	make -r haribote.img

run :
	make img
	qemu-system-i386 -fda haribote.img

vdi : haribote.img
	rm ./helloos.vdi -f
	VBoxManage convertfromraw --format VDI haribote.img haribote.vdi

clean :
	rm *.bin *.o *.lst hankaku.c conv_hankaku *.img *.hrb *.sys

debug:
	make img
	qemu-system-i386 -fda haribote.img -gdb tcp::100000 -S
