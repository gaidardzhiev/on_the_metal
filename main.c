#include <stddef.h>
#include <stdint.h>

static uint32_t MMIO_BASE;

static inline void mmio_init(int raspi) {
	switch (raspi) {
	case 2:
	case 3:
		MMIO_BASE = 0x3F000000;
		break;
	case 4:
		MMIO_BASE = 0xFE000000;
		break;
	default:
		MMIO_BASE = 0x20000000;
		break;
	}
}

static inline void mmio_write(uint32_t reg, uint32_t data) {
	*(volatile uint32_t*)(MMIO_BASE + reg) = data;
}

static inline uint32_t mmio_read(uint32_t reg) {
	return *(volatile uint32_t*)(MMIO_BASE + reg);
}

static inline void delay(int32_t count) {
	asm volatile("__delay_%=: subs %[count], %[count], #1; bne __delay_%=\n"
		     : "=r"(count): [count]"0"(count) : "cc");
}

enum {
	GPIO_BASE = 0x200000,
	GPPUD = (GPIO_BASE + 0x94),
	GPPUDCLK0 = (GPIO_BASE + 0x98),
	UART0_BASE = (GPIO_BASE + 0x1000), //0x20201000
	UART0_DR     = (UART0_BASE + 0x00),
	UART0_RSRECR = (UART0_BASE + 0x04),
	UART0_FR     = (UART0_BASE + 0x18),
	UART0_ILPR   = (UART0_BASE + 0x20),
	UART0_IBRD   = (UART0_BASE + 0x24),
	UART0_FBRD   = (UART0_BASE + 0x28),
	UART0_LCRH   = (UART0_BASE + 0x2C),
	UART0_CR     = (UART0_BASE + 0x30),
	UART0_IFLS   = (UART0_BASE + 0x34),
	UART0_IMSC   = (UART0_BASE + 0x38),
	UART0_RIS    = (UART0_BASE + 0x3C),
	UART0_MIS    = (UART0_BASE + 0x40),
	UART0_ICR    = (UART0_BASE + 0x44),
	UART0_DMACR  = (UART0_BASE + 0x48),
	UART0_ITCR   = (UART0_BASE + 0x80),
	UART0_ITIP   = (UART0_BASE + 0x84),
	UART0_ITOP   = (UART0_BASE + 0x88),
	UART0_TDR    = (UART0_BASE + 0x8C),

	MBOX_BASE    = 0xB880,
	MBOX_READ    = (MBOX_BASE + 0x00),
	MBOX_STATUS  = (MBOX_BASE + 0x18),
	MBOX_WRITE   = (MBOX_BASE + 0x20)
};

volatile unsigned int  __attribute__((aligned(16))) mbox[9] = {
	9*4, 0, 0x38002, 12, 8, 2, 3000000, 0,0
};

void uart_init(int raspi) {
	mmio_init(raspi);
	mmio_write(UART0_CR, 0x00000000);
	mmio_write(GPPUD, 0x00000000);
	delay(150);
	mmio_write(GPPUDCLK0, (1 << 14) | (1 << 15));
	delay(150);
	mmio_write(GPPUDCLK0, 0x00000000);
	mmio_write(UART0_ICR, 0x7FF);
	if (raspi >= 3) {
		unsigned int r = (((unsigned int)(&mbox) & ~0xF) | 8);
		while ( mmio_read(MBOX_STATUS) & 0x80000000 ) { }
		mmio_write(MBOX_WRITE, r);
		while ( (mmio_read(MBOX_STATUS) & 0x40000000) || mmio_read(MBOX_READ) != r ) { }
	}
	mmio_write(UART0_IBRD, 1);
	mmio_write(UART0_FBRD, 40);
	mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));
	mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
		   (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));
	mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

void uart_putc(unsigned char c) {
	while ( mmio_read(UART0_FR) & (1 << 5) ) { }
	mmio_write(UART0_DR, c);
}

unsigned char uart_getc() {
	while ( mmio_read(UART0_FR) & (1 << 4) ) { }
	return mmio_read(UART0_DR);
}

void uart_puts(const char* str) {
	for (size_t i = 0; str[i] != '\0'; i ++)
		uart_putc((unsigned char)str[i]);
}

#if defined(__cplusplus)
extern "C"
#endif

#ifdef AARCH64
void kernel_main(uint64_t dtb_ptr32, uint64_t x1, uint64_t x2, uint64_t x3)
#else

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
#endif
{
	uart_init(2);
	uart_puts("we_are_on_the_metal...\r\n");

	while (1)
		uart_putc(uart_getc());
}
