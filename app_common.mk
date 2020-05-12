CC := gcc
# CFLAGS := -c -m32 -march=i486 -nostdlib -fno-pic
CFLAGS := -c -m32 -march=i486

default : $(APP).bin

$(APP).bin : $(APP).o ./../apilib/apilib.lib $(OBJ_LIBRARIES) Makefile
	# gcc -c -m32 -march=i486 -nostdlib -fno-pic -T ../api.ls $(APP).o ../apilib/apilib.lib $(OBJ_LIBRARIES) -o $(APP).bin
	ld -m elf_i386 -e HariMain -n -T ../api.ls -static -o $@ $<  $(OBJ_LIBRARIES) ../apilib/apilib.lib

%.o : %.asm Makefile
	nasm -f elf $< -o $@

%.o : %.c Makefile
	# $(CC) $(CFLAGS) $< -o $@
	gcc -Wall -m32 -c -fno-pic -nostdlib -fno-builtin -I../ -I../haribote/ -o $@ $<
	objdump -D $@ > $@.dmp

clean :
	rm *.o
	rm *.bin
