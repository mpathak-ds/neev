/*++
	Copyright 2026 Driftless Software Pvt. Ltd
	
	NEEV Firmware.

	Module Name:

		sync.c

	Description:

		AMD64 sync primitives.

	Author:

		Mayank Pathak (mpathak).
		30/09/2026
--*/

#include <stdint.h>
#include <amd64/spec.h>
#include <adefs.h>
#include <osdef.h>

void arch_atomic_lock_up(void)
{
	amd64_save_and_disable_int();
}

void arch_atomic_free_up(void)
{
	amd64_restore_and_enable_int();
}

void arch_atomic_lock_mp(uint32_t *lock_ptr)
{
	amd64_atomic_lock(lock_ptr);
}

void arch_atomic_free_mp(uint32_t *lock_ptr)
{
	amd64_atomic_release(lock_ptr);
}
