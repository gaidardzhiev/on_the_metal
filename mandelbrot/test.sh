#!/bin/sh

IMG="kernel7.img"
MACHINE="raspi1"
CPU="arm1176"
RAM="256"
SERIAL="-serial stdio"

fcheck() {
	[ ! -f "$1" ] && {
		printf "file %s not found...\n" "$IMG";
		exit 1;
	}
}

fqemu() {
	qemu-system-arm \
		-M "$MACHINE" \
		-cpu "$CPU" \
		-m "$RAM" \
		-kernel "$IMG" \
		"$SERIAL" \
		-display sdl \
		-no-reboot \
		-serial mon:stdio
}

{ fcheck && fqemu; } || exit 1
