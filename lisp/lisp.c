#include <stdint.h>
#include <stddef.h>

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

volatile unsigned int __attribute__((aligned(16))) mbox[9] = {
	9*4, 0, 0x38002, 12, 8, 2, 3000000, 0, 0
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
		while (mmio_read(MBOX_STATUS) & 0x80000000) { }
		mmio_write(MBOX_WRITE, r);
		while ((mmio_read(MBOX_STATUS) & 0x40000000) || mmio_read(MBOX_READ) != r) { }
	}
	mmio_write(UART0_IBRD, 1);
	mmio_write(UART0_FBRD, 40);
	mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));
	mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) |
		   (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));
	mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

void uart_putc(unsigned char c) {
	while (mmio_read(UART0_FR) & (1 << 5)) { }
	mmio_write(UART0_DR, c);
}

unsigned char uart_getc() {
	while (mmio_read(UART0_FR) & (1 << 4)) { }
	return mmio_read(UART0_DR);
}

void uart_puts(const char* str) {
	for (size_t i = 0; str[i] != '\0'; i++)
		uart_putc((unsigned char)str[i]);
}

#define MAX_TOKENS 64
#define MAX_SYM_LEN 16
#define MAX_ENV 64
#define MAX_STACK 128

#define PUTC(c) uart_putc(c)
#define PUTS(s) uart_puts(s)

#define MAX_OBJS 256

typedef enum {T_INT, T_SYM, T_CONS, T_NIL} t;

typedef struct o {
	t t;
	union {
		int i;
		char s[MAX_SYM_LEN];
		struct {
			struct o *c, *d;
		} p;
	};
} o;

o n = {T_NIL, {0}};
o p[MAX_OBJS];
size_t x = 0;

o* a(void) {
	if (x >= MAX_OBJS) {
		PUTS("out of memory...\r\n");
		return &n;
	}
	return &p[x++];
}

typedef struct {
	o *s;
	o *v;
} e;

e m[MAX_ENV];
size_t y = 0;

o *r[MAX_STACK];
size_t w = 0;

void u(o *v) {
	if (w < MAX_STACK) r[w++] = v;
	else PUTS("stack overflow...\r\n");
}

o* q(void) {
	if (w > 0) return r[--w];
	PUTS("stack underflow...\r\n");
	return &n;
}

o* b(int v) {
	o *z = a();
	z->t = T_INT;
	z->i = v;
	return z;
}

o* sym(const char *z) {
	o *c = a();
	c->t = T_SYM;
	size_t i = 0;
	while (z[i] && i < MAX_SYM_LEN-1) {
		c->s[i] = z[i];
		i++;
	}
	c->s[i] = '\0';
	return c;
}

o* cons(o *c, o *d) {
	o *z = a();
	z->t = T_CONS;
	z->p.c = c;
	z->p.d = d;
	return z;
}

#define I(o) ((o)->t == T_NIL)

int S(o *c, o *d) {
	if (c->t != T_SYM || d->t != T_SYM) return 0;
	for (int i = 0; i < MAX_SYM_LEN; i++) {
		if (c->s[i] != d->s[i]) return 0;
		if (c->s[i] == '\0') break;
	}
	return 1;
}

o* g(o *s) {
	for (size_t i = 0; i < y; i++) {
		if (S(m[i].s, s)) return m[i].v;
	}
	return &n;
}

void h(o *s, o *v) {
	for (size_t i = 0; i < y; i++) {
		if (S(m[i].s, s)) {
			m[i].v = v;
			return;
		}
	}
	if (y < MAX_ENV) {
		m[y].s = s;
		m[y].v = v;
		y++;
	} else {
		PUTS("environment full...\r\n");
	}
}

o* k(char **s);
o* l(o *e);

void j(char **s) {
	while (**s == ' ' || **s == '\t' || **s == '\r' || **s == '\n') (*s)++;
}

o* i(char **s) {
	j(s);
	char z[MAX_SYM_LEN];
	int c = 0;
	if (**s == '\0') return &n;
	if (**s == '(' || **s == ')') {
		z[0] = **s;
		z[1] = '\0';
		(*s)++;
		return sym(z);
	}
	while (**s && **s != ' ' && **s != '\t' && **s != '\r' && **s != '\n' && **s != '(' && **s != ')') {
		if (c < MAX_SYM_LEN-1) z[c++] = **s;
		(*s)++;
	}
	z[c] = '\0';
	int neg = 0, v = 0;
	char *p = z;
	if (*p == '-') {
		neg = 1;
		p++;
	}
	if (*p >= '0' && *p <= '9') {
		v = 0;
		while (*p >= '0' && *p <= '9') {
			v = v * 10 + (*p - '0');
			p++;
		}
		if (*p == '\0') return b(neg ? -v : v);
	}
	return sym(z);
}

o* jlist(char **s) {
	j(s);
	if (**s == ')') {
		(*s)++;
		return &n;
	}
	o *c = k(s);
	o *d = jlist(s);
	return cons(c, d);
}

o* k(char **s) {
	j(s);
	if (**s == '(') {
		(*s)++;
		return jlist(s);
	}
	return i(s);
}

void mprint(o *obj) {
	switch (obj->t) {
	case T_INT: {
		char b[12];
		int v = obj->i;
		int i = 10;
		int neg = 0;
		b[11] = '\0';
		if (v == 0) {
			PUTC('0');
			return;
		}
		if (v < 0) {
			neg = 1;
			v = -v;
		}
		while (v > 0 && i >= 0) {
			b[i--] = '0' + (v % 10);
			v /= 10;
		}
		if (neg) b[i--] = '-';
		PUTS(&b[i+1]);
	}
	break;
	case T_SYM:
		PUTS(obj->s);
		break;
	case T_CONS:
		PUTS("(");
		mprint(obj->p.c);
		o *q = obj->p.d;
		while (q->t == T_CONS) {
			PUTC(' ');
			mprint(q->p.c);
			q = q->p.d;
		}
		if (!I(q)) {
			PUTS(" . ");
			mprint(q);
		}
		PUTS(")");
		break;
	case T_NIL:
		PUTS("nil");
		break;
	}
}

o* l(o *expr) {
	if (!expr) return &n;
	switch (expr->t) {
	case T_INT:
	case T_NIL:
		return expr;
	case T_SYM: {
		o *v = g(expr);
		if (v == &n) {
			PUTS("unbound symbol: ");
			PUTS(expr->s);
			PUTS("\r\n");
		}
		return v;
	}
	case T_CONS: {
		o *f = expr->p.c;
		o *a = expr->p.d;

		if (f->t == T_SYM) {
			if (S(f, sym("quote"))) return a->p.c;
			if (S(f, sym("if"))) {
				o *c = l(a->p.c);
				if (c != &n) return l(a->p.d->p.c);
				else return l(a->p.d->p.d->p.c);
			}
			if (S(f, sym("define"))) {
				o *s = a->p.c;
				o *v = l(a->p.d->p.c);
				h(s, v);
				return s;
			}
			if (S(f, sym("+"))) {
				o *x = l(a->p.c);
				o *y = l(a->p.d->p.c);
				if (x->t == T_INT && y->t == T_INT) return b(x->i + y->i);
				PUTS("+ expects two integers...\r\n");
				return &n;
			}
			if (S(f, sym("-"))) {
				o *x = l(a->p.c);
				o *y = l(a->p.d->p.c);
				if (x->t == T_INT && y->t == T_INT) return b(x->i - y->i);
				PUTS("- expects two integers...\r\n");
				return &n;
			}
			if (S(f, sym("*"))) {
				o *x = l(a->p.c);
				o *y = l(a->p.d->p.c);
				if (x->t == T_INT && y->t == T_INT) return b(x->i * y->i);
				PUTS("* expects two integers...\r\n");
				return &n;
			}
			if (S(f, sym("/"))) {
				o *x = l(a->p.c);
				o *y = l(a->p.d->p.c);
				if (x->t == T_INT && y->t == T_INT) {
					if (y->i == 0) {
						PUTS("division by zero...\r\n");
						return &n;
					}
					return b(x->i / y->i);
				}
				PUTS("/ expects two integers...\r\n");
				return &n;
			}
			if (S(f, sym("car"))) {
				o *x = l(a->p.c);
				if (x->t == T_CONS) return x->p.c;
				PUTS("car expects a cons cell...\r\n");
				return &n;
			}
			if (S(f, sym("cdr"))) {
				o *x = l(a->p.c);
				if (x->t == T_CONS) return x->p.d;
				PUTS("cdr expects a cons cell...\r\n");
				return &n;
			}
			if (S(f, sym("cons"))) {
				o *x = l(a->p.c);
				o *y = l(a->p.d->p.c);
				return cons(x, y);
			}
		}
		PUTS("unknown function or form...\r\n");
		return &n;
	}
	}
	return &n;
}

void readline(char *b, size_t m) {
	size_t i = 0;
	while (1) {
		char c = uart_getc();
		if (c == '\r' || c == '\n') {
			PUTS("\r\n");
			b[i] = '\0';
			return;
		}
		if ((c == 8 || c == 127) && i > 0) {
			i--;
			PUTS("\b \b");
			continue;
		}
		if (i < m - 1) {
			b[i++] = c;
			uart_putc(c);
		}
	}
}

#define B 1024

void kernel_main(uint32_t r0 __attribute__((unused)), uint32_t r1 __attribute__((unused)), uint32_t atags __attribute__((unused))) {
	uart_init(2);
	PUTS("bare metal lisp interpreter\r\n");
	PUTS("type lisp expressions and press enter\r\n");
	char b[B];
	while (1) {
		PUTS("> ");
		readline(b, sizeof(b));
		PUTS("input read: ");
		PUTS(b);
		PUTS("\r\n");
		char *s = b;
		o *expr = k(&s);
		if (expr != &n) {
			PUTS("parsed expression\r\n");
			o *v = l(expr);
			PUTS("evaluated expression\r\n");
			mprint(v);
			PUTS("\r\n");
		} else {
			PUTS("parse returned null\r\n");
		}
	}
}
