#ifndef OS_SERCON_DEF_H
#define OS_SERCON_DEF_H

#include <stdint.h>

struct console_ops
{
	void (*putc)(char c);
	int (*getc)(void);
};

struct console
{
	void *device_addr;
	struct console_ops ops;
};

void
console_init (
	void *serial_base,
	struct console_ops *serial_ops
	);

void
console_putc (
	char c
	);

void
console_puts (
	const char *s
	);

int
console_getc (
	void
	);

#endif
