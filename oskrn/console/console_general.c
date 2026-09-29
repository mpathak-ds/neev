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
#include <adefs.h>
#include <console/serial.h>
#include <console/video.h>

static uint8_t g_is_verbose = 0;

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
			return STATUS_FAILED_DEVICE_INIT;
		}

		if (verbose) {
			video_puts("init: Initialized display console");
		}
		console_puts("init: Initialized display console\n");
	}

	g_is_verbose = verbose;

	return STATUS_SUCCESS;
}

void
kputs (
	const char *str
	)
{
	if (g_is_verbose) {
		video_puts(str);
	}

	console_puts(str);
}

static void
itoa_internal (
    uint64_t val,
    int base,
    int is_signed,
    char *buf
)
{
    char tmp[64];
    int i = 0;
    int is_neg = 0;
    const char digits[] = "0123456789abcdef";

    if (is_signed && (int64_t)val < 0) {
        is_neg = 1;
        val = (uint64_t)(-(int64_t)val);
    }

    if (val == 0) {
        tmp[i++] = '0';
    } else {
        while (val > 0) {
            tmp[i++] = digits[val % base];
            val /= base;
        }
    }

    if (is_neg) {
        tmp[i++] = '-';
    }

    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}
