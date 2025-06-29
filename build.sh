#!/bin/sh

arm-none-eabi-gcc -mcpu=arm1176jzf-s -fpic -ffreestanding -c start.s -o start.o

arm-none-eabi-gcc -mcpu=arm1176jzf-s -fpic -ffreestanding -std=gnu99 -c main.c -o main.o -O2 -Wall -Wextra

arm-none-eabi-gcc -T link.ld -o on_the_metal.elf -ffreestanding -O2 -nostdlib start.o main.o -lgcc

arm-none-eabi-objcopy on_the_metal.elf -O binary kernel7.img

qemu-system-aarch64 -M raspi0 -serial stdio -kernel kernel7.img
