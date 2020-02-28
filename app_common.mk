CC := gcc
# CFLAGS := -c -m32 -march=i486 -nostdlib -fno-pic
CFLAGS := -c -m32 -march=i486

default : $(APP).bin

$(APP).bin : $(APP).o ./../apilib/apilib.lib $(OBJ_LIBRARIES) Makefile
	gcc -m32 -march=i486 -nostdlib -fno-pic -T ../api.ls $(APP).o ../apilib/apilib.lib $(OBJ_LIBRARIES) -o $(APP).bin

%.o : %.asm Makefile
	nasm -f elf $< -o $@

%.o : %.c Makefile
	$(CC) $(CFLAGS) $< -o $@

clean :
	rm *.bin *.o
