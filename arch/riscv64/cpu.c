/*++
	Copyright 2026 Driftless Software Pvt. Ltd
	
	NEEV Firmware.

	Module Name:

		cpu.c

	Description:

		RISCV64 CPU primitives.

	Author:

		Mayank Pathak (mpathak).
		30/09/2026
--*/

#include <stdint.h>
#include <adefs.h>
#include <osdef.h>

void arch_disable_interrupts(void)
{
	__asm__ __volatile__("csrc sstatus, %0" : : "r"(1 << 1));
}

void arch_spin_forever(void)
{
	for (;;) {
		__asm__ __volatile__("wfi");
	}
}
