.section ".text.boot"
.globl _start
	.org 0x8000
_start:
	/*stack pointer near top of RAM 128MB */
	ldr sp, =0x8000000
	ldr r4, =__bss_start
	ldr r9, =__bss_end
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0
1:
	stmia r4!, {r5-r8}
2:
	cmp r4, r9
	blo 1b
	ldr r3, =kernel_main
	blx r3
halt:
	wfe
	b halt
