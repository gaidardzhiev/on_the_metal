SHELL=/bin/sh
AS=arm-none-eabi-gcc
CC=arm-none-eabi-gcc
LD=arm-none-eabi-gcc
OBJ=arm-none-eabi-objcopy
ASFLAGS=-mcpu=arm1176jzf-s -fpic -ffreestanding
CFLAGS=-mcpu=arm1176jzf-s -fpic -ffreestanding -std=gnu99 -O2 -Wall -Wextra
LDFLAGS=-ffreestanding -O2 -nostdlib
START=start.S
MAIN=main.c
LINK=link.ld

all:start.o
all:main.o

start.o: $(START)
	$(AS) $(ASFLAGS) -c $(START)  -o start.o

main.o: $(MAIN)
	$(CC) $(CFLAGS) -c $(MAIN) -o main.o

$(SHELL) != $(LD) -T $(LINK) -o on_the_metal.elf $(LDFLAGS) start.o main.o -lgcc
$(SHELL) != $(OBJ) on_the_metal.elf -O binary kernel7.img

clean:
	rm start.o main.o on_the_metal.elf kernel7.img

