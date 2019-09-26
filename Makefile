
# デフォルト動作

default :
	make run

# ファイル生成規則

ipl.bin : ipl.asm Makefile
	nasm ipl.asm -o ipl.bin -l ipl.lst

asmhead.bin : asmhead.asm Makefile
	nasm asmhead.asm -o asmhead.bin -l asmhead.lst

nasmfunc.o : nasmfunc.asm Makefile
	nasm -f elf nasmfunc.asm -o nasmfunc.o -l nasmfunc.lst

hankaku.c : hankaku.txt conv_hankaku
	./conv_hankaku

conv_hankaku : conv_hankaku.c
	gcc -o conv_hankaku conv_hankaku.c

hankaku.o : hankaku.c
	gcc -c -m32 -march=i486 -nostdlib -o hankaku.o hankaku.c

mysprintf.o : mysprintf.c
	gcc -c -m32 -march=i486 -nostdlib mysprintf.c -o mysprintf.o

bootpack.hrb : bootpack.c hankaku.o nasmfunc.o mysprintf.o har.ls Makefile
	gcc -m32 -march=i486 -nostdlib -nostdinc -T har.ls bootpack.c hankaku.o nasmfunc.o mysprintf.o -o bootpack.hrb

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
