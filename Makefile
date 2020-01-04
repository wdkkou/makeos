CC := gcc
CFLAGS := -c -m32 -march=i486 -nostdlib -fno-pic
OBJ_ALL := bootpack.o hankaku.o nasmfunc.o mysprintf.o mystrcmp.o graphic.o dsctbl.o int.o \
				fifo.o mouse.o keyboard.o memory.o sheet.o timer.o mtask.o window.o console.o file.o
# APP_ALL = hello/hello.bin
# APP_ALL += hello2/hello2.bin
# APP_ALL += hello3/hello3.bin
# APP_ALL += hello4/hello4.bin
# APP_ALL += hello5/hello5.bin
APP_ALL += star/star.bin
APP_ALL += stars/stars.bin
APP_ALL += line/line.bin
APP_ALL += walk/walk.bin
# APP_ALL += beepup/beepup.bin
# APP_ALL += noodle/noodle.bin
APP_ALL += color/color.bin
APP_ALL += color2/color2.bin
APP_ALL += sosu/sosu.bin
APP_ALL += sosu2/sosu2.bin
APP_ALL += winhello/win.bin
APP_ALL += winhello2/win2.bin
APP_ALL += winhello3/win3.bin
# APP_ALL += catipl/catipl.bin
APP_ALL += cat/cat.bin
APP_ALL += iroha/iroha.bin
APP_ALL += chklang/chklang.bin
APP_ALL += notrec/notrec.bin
APP_ALL += bball/bball.bin

# デフォルト動作
all :
	make full
	make run

full :
	$(MAKE) -C apilib
	# $(MAKE) -C hello
	# $(MAKE) -C hello2
	# $(MAKE) -C hello3
	# $(MAKE) -C hello4
	# $(MAKE) -C hello5
	$(MAKE) -C winhello
	$(MAKE) -C winhello2
	$(MAKE) -C winhello3
	$(MAKE) -C star
	$(MAKE) -C stars
	$(MAKE) -C line
	$(MAKE) -C walk
	$(MAKE) -C noodle
	# $(MAKE) -C beepup
	$(MAKE) -C color
	$(MAKE) -C color2
	$(MAKE) -C sosu
	$(MAKE) -C sosu2
	# $(MAKE) -C catipl
	$(MAKE) -C cat
	$(MAKE) -C iroha
	$(MAKE) -C chklang
	$(MAKE) -C notrec
	$(MAKE) -C bball
	$(MAKE) -C os

# ファイル生成規則
os/haribote.img :
	$(MAKE) -C os

img : os/haribote.img $(APP_ALL)
	cp os/haribote.img myos.img
	mcopy cat.txt -i myos.img ::
	mcopy $(APP_ALL) -i myos.img ::
	mcopy os/ipl.asm -i myos.img ::
	mcopy ipl10.nas -i myos.img ::
	mcopy nihongo.txt -i myos.img ::
	mcopy euc.txt -i myos.img ::
	mcopy fonts/nihongo.fnt -i myos.img ::

# コマンド

run : img
	qemu-system-i386 -m 32 -fda myos.img

vdi : myos.img
	rm ./myos.vdi -f
	VBoxManage convertfromraw --format VDI myos.img myos.vdi

clean :
	$(MAKE) -C apilib clean
	$(MAKE) -C winhello clean
	$(MAKE) -C winhello2 clean
	$(MAKE) -C winhello3 clean
	$(MAKE) -C star clean
	$(MAKE) -C stars clean
	$(MAKE) -C line clean
	$(MAKE) -C walk clean
	$(MAKE) -C noodle clean
	# $(MAKE) -C beepup clean
	$(MAKE) -C color clean
	$(MAKE) -C color2 clean
	$(MAKE) -C sosu clean
	$(MAKE) -C sosu2 clean
	# $(MAKE) -C catipl clean
	$(MAKE) -C cat clean
	$(MAKE) -C iroha clean
	$(MAKE) -C chklang clean
	$(MAKE) -C notrec clean
	$(MAKE) -C bball clean
	$(MAKE) -C os clean
	rm myos.img

debug:
	make full
	qemu-system-i386 -fda haribote.img -gdb tcp::100000 -S
