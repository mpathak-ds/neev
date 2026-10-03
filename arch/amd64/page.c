/*++
	Copyright 2026 Driftless Software Pvt. Ltd
	
	NEEV Firmware.

	Module Name:

		page.c

	Description:

		AMD64 paging.

	Author:

		Mayank Pathak (mpathak).
		03/10/2026
--*/

#include <stdint.h>
#include <amd64/spec.h>
#include <console/kd.h>
#include <adefs.h>
#include <osdef.h>

void arch_init_mmu(pfirmware_info_t boot_info)
{
	kprintf("init: no paging yet for amd64\n");
}
