/*++
	Copyright 2026 Driftless Software Pvt. Ltd
	
	NEEV Firmware.

	Module Name:

		sync.c

	Description:

		RISCV64 sync primitives.

	Author:

		Mayank Pathak (mpathak).
		30/09/2026
--*/

#include <stdint.h>
#include <adefs.h>
#include <osdef.h>

extern void rv64_atomic_lock_mp(uint32_t *ptr);
extern void rv64_atomic_free_mp(uint32_t *ptr);
extern void rv64_atomic_lock_up(void);
extern void rv64_atomic_free_up(void);

void arch_atomic_lock_up(void)
{
	rv64_atomic_lock_up();
}

void arch_atomic_free_up(void)
{
	rv64_atomic_free_up();
}

void arch_atomic_lock_mp(uint32_t *lock_ptr)
{
	rv64_atomic_lock_mp(lock_ptr);
}

void arch_atomic_free_mp(uint32_t *lock_ptr)
{
	rv64_atomic_free_mp(lock_ptr);
}
