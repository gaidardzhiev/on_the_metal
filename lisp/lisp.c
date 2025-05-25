#include <stdint.h>
#include <stddef.h>

#define NULL 0
#define MAX_TOKENS 64
#define MAX_SYM_LEN 16
#define MAX_ENV 64
#define MAX_STACK 128

extern void uart_init(int z);
extern void uart_putc(unsigned char c);
extern unsigned char uart_getc(void);
extern void uart_puts(const char* s);

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

o* e(const char *z) {
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

o* f(o *c, o *d) {
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
		return e(z);
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
	return e(z);
}

o* jlist(char **s) {
	j(s);
	if (**s == ')') {
		(*s)++;
		return &n;
	}
	o *c = k(s);
	o *d = jlist(s);
	return f(c, d);
}

o* k(char **s) {
	j(s);
	if (**s == '(') {
		(*s)++;
		return jlist(s);
	}
	return i(s);
}

void mprint(o *o) {
	switch (o->t) {
	case T_INT: {
		char b[12];
		int v = o->i;
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
		PUTS(o->s);
		break;
	case T_CONS:
		PUTS("(");
		mprint(o->p.c);
		o *q = o->p.d;
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

o* l(o *e) {
	if (!e) return &n;
	switch (e->t) {
	case T_INT:
	case T_NIL:
		return e;
	case T_SYM: {
		o *v = g(e);
		if (v == &n) {
			PUTS("unbound symbol: ");
			PUTS(e->s);
			PUTS("\r\n");
		}
		return v;
	}
	case T_CONS: {
		o *f = e->p.c;
		o *a = e->p.d;

		if (f->t == T_SYM) {
			if (S(f, e("quote"))) return a->p.c;
			if (S(f, e("if"))) {
				o *c = l(a->p.c);
				if (c != &n) return l(a->p.d->p.c);
				else return l(a->p.d->p.d->p.c);
			}
			if (S(f, e("define"))) {
				o *s = a->p.c;
				o *v = l(a->p.d->p.c);
				h(s, v);
				return s;
			}
			if (S(f, e("+"))) {
				o *x = l(a->p.c);
				o *y = l(a->p.d->p.c);
				if (x->t == T_INT && y->t == T_INT) return b(x->i + y->i);
				PUTS("+ expects two integers...\r\n");
				return &n;
			}
			if (S(f, e("-"))) {
				o *x = l(a->p.c);
				o *y = l(a->p.d->p.c);
				if (x->t == T_INT && y->t == T_INT) return b(x->i - y->i);
				PUTS("- expects two integers...\r\n");
				return &n;
			}
			if (S(f, e("*"))) {
				o *x = l(a->p.c);
				o *y = l(a->p.d->p.c);
				if (x->t == T_INT && y->t == T_INT) return b(x->i * y->i);
				PUTS("* expects two integers...\r\n");
				return &n;
			}
			if (S(f, e("/"))) {
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
			if (S(f, e("car"))) {
				o *x = l(a->p.c);
				if (x->t == T_CONS) return x->p.c;
				PUTS("car expects a cons cell...\r\n");
				return &n;
			}
			if (S(f, e("cdr"))) {
				o *x = l(a->p.c);
				if (x->t == T_CONS) return x->p.d;
				PUTS("cdr expects a cons cell...\r\n");
				return &n;
			}
			if (S(f, e("cons"))) {
				o *x = l(a->p.c);
				o *y = l(a->p.d->p.c);
				return f(x, y);
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

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags) {
	uart_init(3);
	PUTS("bare metal lisp interpreter\r\ntype lisp expressions and press enter\r\n");
	char b[B];
	while (1) {
		PUTS("> ");
		readline(b, sizeof(b));
		char *s = b;
		o *e = k(&s);
		if (e != &n) {
			o *v = l(e);
			mprint(v);
			PUTS("\r\n");
		}
	}
}
