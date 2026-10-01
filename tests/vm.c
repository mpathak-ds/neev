/*
 * NEEV Kernel Test Suite
 * Path: vm.c
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
#include <console/kd.h>
#include <osdef.h>
#include <vm/pm.h>

os_status
test_physmem (
	pfirmware_info_t boot_info
	)
{
	kprintf("\ntest_physmem: beginning physical memory tests\n");

	// try allocating 1kb
	uint64_t test = balloc(1024);

	if (!test) {
		// allocation failed
		kprintf("test_physmem: failed to allocate 1024 bytes, returned NULL address\n");
		return STATUS_FAILED_EARLY_TEST;
	}
	
	kprintf("test_physmem: allocated memory of 1024 bytes at 0x%lx\n", test);
	// certain loaders sets up higher half mapping for us, need to add their offsets
	test += boot_info->fw_virt_offset;
	kprintf("test_physmem: writing value 0xdeadbeef to allocated address\n");
	// try writing magic
	*(volatile uint64_t*)test = 0xdeadbeef;

	if (*(volatile uint64_t*)test != 0xdeadbeef) {
		// unexpected value found
		kprintf("test_physmem: write was not persistent, found value 0x%lx instead\n", *(volatile uint64_t*)test);
		return STATUS_FAILED_EARLY_TEST;
	}
	
	kprintf("test_physmem: wrote, persistence: 0x%lx\n", *(volatile uint64_t*)test);

	// all good
	kprintf("test_physmem: test successful!\n");
	return STATUS_SUCCESS;
}
