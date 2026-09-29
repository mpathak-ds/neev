/*
 * NEEV Kernel
 * Path: console/serial_console.c
 *
 * Copyright (c) 2026 Driftless Software. All rights reserved.
 * Property of Driftless Software.
 *
 * NOTICE: This software is governed by a license agreement. 
 * Redistribution, modification, or use of this file, in whole or in part,
 * is strictly restricted to the terms specified in the 'LICENSE' file 
 * located at the root directory of this project repository.
 *
 * Author: Mayank Pathak (mpathak)
 */

#include <stdint.h>
#include <adefs.h>
#include <console/serial.h>

struct console g_console;

void
console_init (
	void *serial_base,
	struct console_ops *serial_ops
	)

/*
	Function Description:

		This function initializes the generic serial console.

	Function Parameters:

		serial_base - Memory mapped physical address of the serial.
		serial_ops  - Pointer to the filled function table.

	Function Return:

		None.

	Function Notes:

		None.
*/
	
{
	g_console.device_addr = serial_base;
	g_console.ops = *serial_ops;
}

void
console_putc (
	char c
	)
{
	if (!g_console.ops.putc) return;

	if (c == '\n') {
		g_console.ops.putc('\r');
	}

	g_console.ops.putc(c);
}

void
console_puts (
	const char *s
	)
{
	if (!s) return;

	while (*s) {
		console_putc(*s);
		s++;
	}
}

int
console_getc (
	void
	)
{
	if (!g_console.ops.getc) return -1;

	return g_console.ops.getc();
}
