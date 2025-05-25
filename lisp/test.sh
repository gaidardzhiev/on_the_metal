#!/bin/sh

IMG="kernel7.img"

[ ! -f "$IMG" ] && {
	printf "$IMG not found...\n";
	exit 1;
}

qemu-system-arm \
	-M raspi \
	-cpu arm1176 \
	-m 256 \
	-kernel "$IMG" \
	-serial stdio \
	-display sdl \
	-no-reboot
