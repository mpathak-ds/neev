/*++
	Copyright 2026 Driftless Software Pvt. Ltd
	
	NEEV Firmware.

	Module Name:

		adefs.h

	Description:

		Generic header file template for the microkernel to use for arch-specific functionality.

	Author:

		Mayank Pathak (mpathak).
		24/09/2026
--*/

#ifndef ARCH_SPEC_DEF_H
#define ARCH_SPEC_DEF_H

#include <stdint.h>
#include <console/video.h>
#include <console/serial.h>

//#define FW_CONFIG_IS_MCU

typedef struct _firmware_info
{
	//in bytes
	uint64_t fw_total_ram;
	uint64_t fw_ram_base;
	uint32_t fw_core_num;
	
	uint64_t fw_ser_base;
	struct console_ops *fw_ser_ops;
	uint8_t fw_is_video;
	struct video_console *fw_vid_info;
} firmware_info_t, *pfirmware_info_t;

uint32_t
clock_init (
	uint32_t ser_baud,
	uint32_t timer_rate
	);

void
neev_init (
	pfirmware_info_t boot_info
	);

void early_putc(char c);
void early_puts(const char *s);
void early_puthex(uint64_t v);

#endif
