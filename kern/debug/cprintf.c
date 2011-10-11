// Implementation of cprintf console output for user environments,
// based on printfmt() and cputs().
//
// cprintf is a debugging facility, not a generic output facility.
// It is very important that it always go to the console, especially when
// debugging file descriptor code!

#include <architecture/types.h>
#include <inc/stdarg.h>

#include <kern/debug/console.h>
#include <kern/debug/stdio.h>


// Collect up to CONSBUFSIZE-1 characters into a buffer
// and perform ONE system call to print all of them,
// in order to make the lines output to the console atomic
// and prevent interrupts from causing context switches
// in the middle of a console output line and such.
struct printbuf {
	int idx;	// current buffer index
	int cnt;	// total bytes printed so far
	char buf[CONSBUFSIZE];
};


static void
putch(int ch, struct printbuf *b)
{
	b->buf[b->idx++] = ch;
	if (b->idx == CONSBUFSIZE-1) {
		b->buf[b->idx] = 0;
		cputs(b->buf);
		b->idx = 0;
	}
	b->cnt++;
}

int
vcprintf(const char *fmt, va_list ap)
{
	struct printbuf b;

	b.idx = 0;
	b.cnt = 0;
	vprintfmt((void*)putch, &b, fmt, ap);

	b.buf[b.idx] = 0;
	cputs(b.buf);

	return b.cnt;
}

int
cprintf(const char *fmt, ...)
{
	va_list ap;
	int cnt;

	static volatile int printing = 0;

	while (printing);
	printing = ~0;
	va_start(ap, fmt);
	cnt = vcprintf(fmt, ap);
	va_end(ap);
	printing = 0;

	return cnt;
}
