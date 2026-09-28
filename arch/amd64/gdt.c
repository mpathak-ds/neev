/*++
	Copyright 2026 Driftless Software Pvt. Ltd
	
	NEEV Firmware.

	Module Name:

		gdt.c

	Description:

		AMD64 GDT setup.

	Author:

		Mayank Pathak (mpathak).
		28/09/2026
--*/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <amd64/spec.h>
#include <adefs.h>

static uint64_t gdt[3] = {
	0x0000000000000000ULL,
	0x00209A0000000000ULL,
	0x0000920000000000ULL
};

static gdtr g_gdtr;

extern void amd64_load_gdt(const void *gdtr);

void amd64_init_gdt(void)
{
	g_gdtr.lim = sizeof(gdt) - 1;
	g_gdtr.base = (uint64_t)gdt;
	amd64_load_gdt(&g_gdtr);
}
