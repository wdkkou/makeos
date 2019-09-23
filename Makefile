
# デフォルト動作

default :
	make img

# ファイル生成規則

ipl.bin : ipl.asm Makefile
	nasm ipl.asm -o ipl.bin -l ipl.lst

asmhead.bin : asmhead.asm Makefile
	nasm asmhead.asm -o asmhead.bin -l asmhead.lst

nasmfunc.o : nasmfunc.asm Makefile
	nasm -g -f elf32 nasmfunc.asm -o nasmfunc.o -l nasmfunc.lst

bootpack.hrb : bootpack.c har.ls nasmfunc.o Makefile
	gcc -m32 -nostdlib -T har.ls -g bootpack.c nasmfunc.o -o bootpack.hrb

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
	rm *.bin
	rm *.lst
	rm *.sys
	rm *.img
	rm *.hrb
	rm *.o

debug:
	make img
	qemu-system-i386 -fda haribote.img -gdb tcp::100000 -S
