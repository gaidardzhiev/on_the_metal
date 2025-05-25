AS=arm-none-eabi-gcc
CC=arm-none-eabi-gcc
LD=arm-none-eabi-gcc
OBJ=arm-none-eabi-objcopy
ASFLAGS=-mcpu=arm1176jzf-s -ffreestanding
CFLAGS=-mcpu=arm1176jzf-s -ffreestanding -std=gnu99 -O2 -Wall -Wextra
LDFLAGS=-T link.ld -ffreestanding -O2 -nostdlib
START=mandelstart.s
MAIN=mandelbrot.c
LINK=link.ld
OBJS=start.o main.o

all: kernel7.img

start.o: $(START)
	$(AS) $(ASFLAGS) -c $< -o $@

main.o: $(MAIN)
	$(CC) $(CFLAGS) -c $< -o $@

on_the_metal.elf: start.o main.o $(LINK)
	$(LD) $(LDFLAGS) -o $@ start.o main.o -lgcc

kernel7.img: on_the_metal.elf
	$(OBJ) $< -O binary $@

clean:
	rm -f *.o on_the_metal.elf kernel7.img

