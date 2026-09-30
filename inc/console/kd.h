#ifndef OS_KDCON_DEF_H
#define OS_KDCON_DEF_H

#include <stdint.h>
#include <stdarg.h>
#include <adefs.h>

#if defined(__GNUC__) || defined(__clang__)
#define KPRINTF_FORMAT(fmt_idx, arg_idx) \
	__attribute__((format(printf, fmt_idx, arg_idx)))
#else
#define KPRINTF_FORMAT(fmt_idx, arg_idx)
#endif

os_status
kd_init (
	pfirmware_info_t boot_info,
	uint8_t verbose
);

void
kputs (
	const char *str
	);

uint8_t
kd_is_online (
	void
	);

int
kprintf (
	const char *fmt,
	...
	) KPRINTF_FORMAT(1, 2);

int
kvprintf (
	const char *fmt,
	va_list ap
	);

void
kpanic (
	panic_code cause,
	uintptr_t params,
	const char *fmt,
	...
	);
	
#endif
