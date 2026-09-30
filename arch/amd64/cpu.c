/*++
	Copyright 2026 Driftless Software Pvt. Ltd
	
	NEEV Firmware.

	Module Name:

		cpu.c

	Description:

		AMD64 CPU wrappers.

	Author:

		Mayank Pathak (mpathak).
		30/09/2026
--*/

#include <stdint.h>
#include <amd64/spec.h>
#include <adefs.h>
#include <osdef.h>

void hcf()
{
	for (;;) {
		asm("hlt");
	}
}

void arch_disable_interrupts(void)
{
	asm("cli");
}

void arch_spin_forever(void)
{
	hcf();
}
