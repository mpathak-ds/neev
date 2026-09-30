/*
 * NEEV Kernel
 * Path: kern/debug.c
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
#include <adefs.h>
#include <osdef.h>
#include <libstr.h>

//
// This module implements a simple panic and prepares the system for
// debugging incase.
//

void
panic_cause_name (
	panic_code cause,
	char *buffer
	)
{
	//
	// Finds human readable string version of error code
	//

	switch (cause) {
		case PANIC_FAILED_EARLY_INIT:
			strcpy(buffer, "Failed early initialization");
			break;

		case PANIC_FAILED_LATE_INIT:
			strcpy(buffer, "Failed further initialization");
			break;

		case PANIC_INVALID_CALL:
			strcpy(buffer, "Invalid parameters or call");
			break;

		case PANIC_UR_PAGE_FAULT:
			strcpy(buffer, "Unrecoverable page fault");
			break;
	
		default:
			strcpy(buffer, "Unknown");
			break;
	}
}

void
panic (
	panic_code cause,
	uintptr_t params,
	const char *fmt,
	...
	)

/*
	Function Description:

		This is called when an unrecoverable error occurs and will display the cause,
		additional parameters and optionally connect to a debugger and gracefully hang
		the system.

	Function Parameters:

		cause     - Panic code and cause of the error.
		params    - Pointer to a structure containing any additional parameters.
		fmt (...) - Additional message.

	Function Return:

		None.

	Function Notes:

		This should ONLY be called if the error is truely unrecoverable.
*/
	
{
	char buffer[100];

	kprintf("\n!!! panic !!!\n");
	kprintf(fmt);
	panic_cause_name(cause, buffer);
	kprintf("\n\nmachine information:\n%s (0x%x)\nparams: 0x%lx (if a debugger is connected, inspect this address for information)\n",
	buffer, cause, params);

	// TODO: display reg info

	arch_disable_interrupts();
	arch_spin_forever();

	while(1);
}
