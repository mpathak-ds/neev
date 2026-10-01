/*
 * NEEV Kernel Test Suite
 * Path: all.c
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
#include <tests/all.h>

os_status
tests_do_all (
	pfirmware_info_t boot_info
	)
{
#if defined(OS_CONDUCT_TESTS)
	os_status status;

	// physmem first
	status = test_physmem(boot_info);

	return status;
#else
	return STATUS_SUCCESS;
#endif
}
