#ifndef OS_KDCON_DEF_H
#define OS_KDCON_DEF_H

#include <stdint.h>
#include <adefs.h>

os_status
kd_init (
	pfirmware_info_t boot_info,
	uint8_t verbose
);

void
kputs (
	const char *str
	);
	
#endif
