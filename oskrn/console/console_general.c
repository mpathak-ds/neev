/*
 * NEEV Kernel
 * Path: console/console_general.c
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
#include <stddef.h>
#include <stdarg.h>
#include <adefs.h>
#include <console/serial.h>
#include <console/video.h>

#define KP_BUF_SIZE 128
#define KP_LEFT 0x01
#define KP_PLUS 0x02
#define KP_SPACE 0x04
#define KP_ZERO 0x08
#define KP_ALT 0x10

enum {
	KP_LEN_HH,
	KP_LEN_H,
	KP_LEN_DEFAULT,
	KP_LEN_L
};

struct kp_out
{
	char buffer[KP_BUF_SIZE];
	uint64_t len;
	int total;
};

static uint8_t g_is_verbose = 0;
static uint8_t g_is_initialized = 0;

os_status
kd_init (
	pfirmware_info_t boot_info,
	uint8_t verbose
	)
{
	os_status status;

	console_init(boot_info->fw_ser_base, boot_info->fw_ser_ops);
	console_puts("init: Console initialized\n");

	if (boot_info->fw_is_video) {
		status = video_console_init(boot_info->fw_vid_info);

		if (status != STATUS_SUCCESS) {
			console_puts("init: Failed to initialize video\n");
			g_is_initialized = 0;
			return STATUS_FAILED_DEVICE_INIT;
		}

		if (verbose) {
			video_puts("init: Initialized display console");
		}
		console_puts("init: Initialized display console\n");
		g_is_verbose = verbose;
	}

	g_is_initialized = 1;

	return STATUS_SUCCESS;
}

uint8_t
kd_is_online (
	void
	)
{
	return g_is_initialized;
}

void
kputs (
	const char *str
	)
{
	// if we are in verbose mode then we should also display
	// on video
	if (g_is_verbose) {
		video_puts(str);
	}

	console_puts(str);
}

static void
kp_flush (
	struct kp_out *o
	)
{
	if (!o->len) {
		return;
	}

	o->buffer[o->len] = '\0';
	kputs(o->buffer);
	o->len = 0;
}

static void
kp_putc (
	struct kp_out *o,
	char c
	)
{
	if (c == '\0') {
		return;
	}

	if (o->len >= KP_BUF_SIZE - 1) {
		kp_flush(o);
	}

	o->buffer[o->len++] = c;
	o->total++;
}

static void
kp_repeat (
	struct kp_out *o,
	char c,
	int n
	)
{
	while (n-- > 0) {
		kp_putc(o, c);
	}
}

static void
kp_string (
	struct kp_out *o,
	const char *s,
	int prec,
	int width,
	uint32_t flags
	)
{
	int len = 0;

	if (s == NULL) {
		s = "(null)";
	}

	while (s[len] != '\0' && (prec<0 || len<prec)) {
		len++;
	}

	if (!(flags & KP_LEFT)) {
		kp_repeat(o, ' ', width-len);
	}

	for (int i = 0; i<len; i++) {
		kp_putc(o, s[i]);
	}

	if (flags & KP_LEFT) {
		kp_repeat(o, ' ', width-len);
	}
}

static void
kp_number (
	struct kp_out *o,
	uint64_t mag,
	uint32_t base,
	int upper,
	char sign,
	uint32_t flags,
	int width,
	int prec
	)
{
	const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
	char tmp[64];
	char prefix[2];
	int nprefix = 0;
	int ndigits = 0;
	int zeros;
	int total;
	int pad;

	if (mag == 0) {
		if (prec != 0) {
			tmp[ndigits++] = '0';
		}
	} else {
		while (mag != 0) {
			tmp[ndigits++] = digits[mag % base];
			mag /= base;
		}
	}

	if (sign != 0) {
		prefix[nprefix++] = sign;
	}
	if ((flags & KP_ALT) && base != 10) {
		if (base == 16 || base == 2) {
			if (ndigits > 0 && !(ndigits == 1 && tmp[0] == '0')) {
				prefix[nprefix++] = '0';
				prefix[nprefix++] = (base == 16) ? (upper ? 'X' : 'x') : 'b';
			}
		}
	}

	zeros = (prec > ndigits) ? (prec - ndigits) : 0;

	if ((flags & KP_ALT) && base == 8 && zeros == 0 &&
		(ndigits == 0 || tmp[ndigits - 1] != '0')) {
		zeros = 1;
	}

	total = nprefix + zeros + ndigits;
	pad   = (width > total) ? (width - total) : 0;

	if ((flags & KP_ZERO) && !(flags & KP_LEFT) && prec < 0) {
		zeros += pad;
		pad = 0;
	}

	if (!(flags & KP_LEFT)) {
		kp_repeat(o, ' ', pad);
	}
	for (int i = 0; i < nprefix; i++) {
		kp_putc(o, prefix[i]);
	}
	kp_repeat(o, '0', zeros);
	while (ndigits > 0) {
		kp_putc(o, tmp[--ndigits]);
	}
	if (flags & KP_LEFT) {
		kp_repeat(o, ' ', pad);
	}
}

int
kvprintf (
	const char *fmt,
	va_list ap
	)
{
	struct kp_out out;
	out.len = 0;
	out.total = 0;

	if (fmt == NULL) {
		return 0;
	}

	for (; *fmt != '\0'; fmt++) {
		uint32_t flags = 0;
		int width = 0;
		int prec = -1;
		int len_mod = KP_LEN_DEFAULT;
		int parsing;

		if (*fmt != '%') {
			kp_putc(&out, *fmt);
			continue;
		}
		fmt++;

		parsing = 1;
		while (parsing) {
			switch (*fmt) {
			case '-': flags |= KP_LEFT; fmt++; break;
			case '+': flags |= KP_PLUS; fmt++; break;
			case ' ': flags |= KP_SPACE; fmt++; break;
			case '0': flags |= KP_ZERO; fmt++; break;
			case '#': flags |= KP_ALT; fmt++; break;
			default:  parsing = 0; break;
			}
		}

		if (*fmt == '*') {
			width = va_arg(ap, int);
			if (width < 0) {
				flags |= KP_LEFT;
				width = -width;
			}
			fmt++;
		} else {
			while (*fmt >= '0' && *fmt <= '9') {
				width = width * 10 + (*fmt - '0');
				fmt++;
			}
		}

		if (*fmt == '.') {
			fmt++;
			prec = 0;
			if (*fmt == '*') {
				prec = va_arg(ap, int);
				fmt++;
			} else {
				while (*fmt >= '0' && *fmt <= '9') {
					prec = prec * 10 + (*fmt - '0');
					fmt++;
				}
			}
		}

		switch (*fmt) {
			case 'h':
				fmt++;
				if (*fmt == 'h') { len_mod = KP_LEN_HH; fmt++; }
				else { len_mod = KP_LEN_H; }
				break;
			case 'l':
				fmt++;
				if (*fmt == 'l') { fmt++; }
				len_mod = KP_LEN_L;
				break;
			case 'z': case 'j': case 't':
				len_mod = KP_LEN_L;
				fmt++;
				break;
			default:
				break;
		}

		switch (*fmt) {
			case '%':
				kp_putc(&out, '%');
				break;

			case 'c': {
				char c = (char)va_arg(ap, int);
				if (!(flags & KP_LEFT)) {
					kp_repeat(&out, ' ', width - 1);
				}
				kp_putc(&out, c);
				if (flags & KP_LEFT) {
					kp_repeat(&out, ' ', width - 1);
				}
				break;
			}

			case 's':
				kp_string(&out, va_arg(ap, const char *), prec, width, flags);
				break;

			case 'd':
			case 'i': {
				int64_t v;
				uint64_t mag;
				char sign = 0;

				switch (len_mod) {
				case KP_LEN_HH: v = (signed char)va_arg(ap, int); break;
				case KP_LEN_H: v = (short)va_arg(ap, int); break;
				case KP_LEN_L: v = va_arg(ap, long long); break;
				default: v = va_arg(ap, int); break;
				}

				if (v < 0) {
					sign = '-';
					mag = 0 - (uint64_t)v;
				} else {
					mag = (uint64_t)v;
					if (flags & KP_PLUS) sign = '+';
					else if (flags & KP_SPACE) sign = ' ';
				}
				kp_number(&out, mag, 10, 0, sign, flags, width, prec);
				break;
			}

			case 'u':
			case 'x':
			case 'X':
			case 'o':
			case 'b': {
				uint64_t v;
				uint32_t base;

				switch (len_mod) {
				case KP_LEN_HH: v = (unsigned char)va_arg(ap, unsigned int); break;
				case KP_LEN_H:  v = (unsigned short)va_arg(ap, unsigned int); break;
				case KP_LEN_L:  v = va_arg(ap, unsigned long long); break;
				default:        v = va_arg(ap, unsigned int); break;
				}

				switch (*fmt) {
					case 'o': base = 8; break;
					case 'b': base = 2; break;
					case 'u': base = 10; break;
					default: base = 16; break;
				}
				kp_number(&out, v, base, *fmt == 'X', 0, flags, width, prec);
				break;
			}

			case 'p': {
				uintptr_t p = (uintptr_t)va_arg(ap, void *);
				if (p == 0) {
					kp_string(&out, "(nil)", -1, width, flags);
				} else {
					kp_number(&out, (uint64_t)p, 16, 0, 0, flags | KP_ALT, width, prec);
				}
				break;
			}

			case '\0':
				fmt--;
				break;

			default:
				kp_putc(&out, '%');
				kp_putc(&out, *fmt);
				break;
		}
	}

	kp_flush(&out);
	return out.total;
}

int
kprintf (
	const char *fmt,
	...
	)
{
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = kvprintf(fmt, ap);
	va_end(ap);

	return n;
}
