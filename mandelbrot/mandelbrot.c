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
	UART0_BASE = (GPIO_BASE + 0x1000),
	UART0_DR     = (UART0_BASE + 0x00),
	UART0_FR     = (UART0_BASE + 0x18),
	UART0_ICR    = (UART0_BASE + 0x44),
	UART0_IBRD   = (UART0_BASE + 0x24),
	UART0_FBRD   = (UART0_BASE + 0x28),
	UART0_LCRH   = (UART0_BASE + 0x2C),
	UART0_CR     = (UART0_BASE + 0x30),
	UART0_IMSC   = (UART0_BASE + 0x38),
	MBOX_BASE    = 0xB880,
	MBOX_READ    = (MBOX_BASE + 0x00),
	MBOX_STATUS  = (MBOX_BASE + 0x18),
	MBOX_WRITE   = (MBOX_BASE + 0x20)
};

//mailbox property interface for framebuffer init
volatile unsigned int __attribute__((aligned(16))) mbox[36];

static int mailbox_call(unsigned int channel) {
	unsigned int r = (((unsigned int)((uintptr_t)mbox) & ~0xF) | (channel & 0xF));
	while (mmio_read(MBOX_STATUS) & 0x80000000) { }//wait until we can write
	mmio_write(MBOX_WRITE, r);
	while (1) {
		while (mmio_read(MBOX_STATUS) & 0x40000000) { }//wait for response
		if (mmio_read(MBOX_READ) == r)
			return mbox[1] == 0x80000000;
	}
	return 0;
}

typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t isrgb;
	uint8_t* buffer;
} framebuffer_t;

framebuffer_t fb;

int framebuffer_init(int width, int height, int depth) {
	mbox[0] = 36 * 4;//total size
	mbox[1] = 0;
	mbox[2] = 0x48003;//physical width height
	mbox[3] = 8;
	mbox[4] = 8;
	mbox[5] = width;
	mbox[6] = height;
	mbox[7] = 0x48004;//virtual width height
	mbox[8] = 8;
	mbox[9] = 8;
	mbox[10] = width;
	mbox[11] = height;
	mbox[12] = 0x48005;//depth
	mbox[13] = 4;
	mbox[14] = 4;
	mbox[15] = depth;
	mbox[16] = 0x48006;//pixel order RGB
	mbox[17] = 4;
	mbox[18] = 4;
	mbox[19] = 1;
	mbox[20] = 0x40001;//allocate framebuffer
	mbox[21] = 8;
	mbox[22] = 8;
	mbox[23] = 16;//alignment
	mbox[24] = 0;
	mbox[25] = 0;
	mbox[26] = 0x40008;//get pitch
	mbox[27] = 4;
	mbox[28] = 4;
	mbox[29] = 0;
	mbox[30] = 0;//end
	mbox[31] = 0;
	if (!mailbox_call(1)) {
		return 0;
	}
	fb.width = mbox[5];
	fb.height = mbox[6];
	fb.buffer = (uint8_t*)(uintptr_t)(mbox[23] & 0x3FFFFFFF);
	fb.pitch = mbox[29];
	fb.isrgb = mbox[19];
	return 1;
}

void put_pixel(int x, int y, uint32_t color) {
	if (x < 0 || x >= (int)fb.width || y < 0 || y >= (int)fb.height)
		return;
	uint32_t* pixel = (uint32_t*)(fb.buffer + y * fb.pitch + x * 4);
	*pixel = color;
}

uint32_t mandelbrot_color(int iter, int max_iter) {
	if (iter == max_iter) return 0xFF000000;
	uint8_t c = (uint8_t)(255 * iter / max_iter);
	return 0xFF000000 | (c << 16) | (c << 8) | c;
}

void draw_mandelbrot() {
	const int max_iter = 1000;
	const double xmin = -2.0;
	const double xmax = 1.0;
	const double ymin = -1.2;
	const double ymax = 1.2;
	for (int py = 0; py < (int)fb.height; py++) {
		for (int px = 0; px < (int)fb.width; px++) {
			double x0 = xmin + (xmax - xmin) * px / fb.width;
			double y0 = ymin + (ymax - ymin) * py / fb.height;
			double x = 0.0;
			double y = 0.0;
			int iter = 0;
			while (x*x + y*y <= 4.0 && iter < max_iter) {
				double xtemp = x*x - y*y + x0;
				y = 2*x*y + y0;
				x = xtemp;
				iter++;
			}
			uint32_t color = mandelbrot_color(iter, max_iter);
			put_pixel(px, py, color);
		}
	}
}

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
	uart_init(3);
	uart_puts("mandelbrot calculationon the metal...\r\n");
	if (!framebuffer_init(640, 480, 32)) {
		uart_puts("failed to initialize framebuffer...\r\n");
		while(1);
	}

	draw_mandelbrot();

	uart_puts("done...\r\n");

	while (1) {
		asm volatile("wfe");
	}
}
