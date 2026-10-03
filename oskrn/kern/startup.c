/*
 * NEEV Kernel
 * Path: kern/startup.c
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
#include <console/serial.h>
#include <adefs.h>
#include <osdef.h>
#include <console/kd.h>
#include <vm/pm.h>
#include <tests/all.h>

#define KD_VERBOSE_MODE 1

void
neev_init (
	pfirmware_info_t boot_info
	)

/*
	Function Description:

		This function initializes the microkernel generically and is called by the
		bootloader or firmware.

	Function Parameters:

		boot_info - Boot information structure.

	Function Return:

		None.

	Function Notes:

		None.
*/
	
{
	uint32_t status = 0;

	(void)boot_info;

	early_puts("\nDriftless Neev Kernel Version 1.0.0.001\n");

	early_puts("init: Initializing peripherals\n");	
	status = clock_init(115200, 1);

	if (status != STATUS_SUCCESS) {
		// cant use kernel panic here.. console is still not initialized
		early_puts("init: Failed early initialization\n");
		while(1);
	}

	//
	// Initialize console for debugging
	//

	status = kd_init(boot_info, KD_VERBOSE_MODE);

	if (status != STATUS_SUCCESS) {
		early_puts("init: Failed early initialization\n");
		while(1);
	}

	//
	// Initialize memory managers
	//

	uint64_t ram_end = boot_info->fw_ram_base + boot_info->fw_total_ram;
	uint64_t ram_start = boot_info->fw_ram_base + boot_info->fw_usable_ram_offset;
	
	kprintf("\ninit: Total %luMB of memory (0x%lx - 0x%lx), running on core %d, %s", boot_info->fw_total_ram / 1048576, boot_info->fw_ram_base,
	boot_info->fw_ram_base+boot_info->fw_total_ram, boot_info->fw_core_num, boot_info->fw_is_video ? "video available" : "no video");

	kprintf("\ninit: Initializing boot allocator\n");
	balloc_init(ram_start, ram_end-ram_start);

	kprintf("init: Initializing frame allocator\n");
	uint64_t boot_end = falloc_init(ram_end, boot_info->fw_virt_offset);

	// mark all of ram free
	falloc_mark_free(boot_end, ram_end-boot_end);

	//
	// Test if enabled
	//

	status = tests_do_all(boot_info);

	if (status != STATUS_SUCCESS) {
		// panic
		panic(PANIC_FAILED_EARLY_INIT, boot_info, "failed test suite with status 0x%lx", status);
	}

	arch_init_mmu(boot_info);

	while(1);
}
