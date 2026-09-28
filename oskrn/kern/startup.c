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
#include <adefs.h>

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
	(void)boot_info;

	early_puts("\nDriftless Neev Kernel Version 1.0.0.001\n");

	early_puts("init: Initializing peripherals\n");
	clock_init(115200, 10000000);
	
	while(1);
}
