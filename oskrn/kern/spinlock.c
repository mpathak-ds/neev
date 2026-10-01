/*
 * NEEV Kernel
 * Path: kern/spinlock.c
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
#include <sync.h>
#include <adefs.h>

void
spinlock_init (
	spinlock_t *lock
	)
{
	*lock = 0;
}

void
spinlock_acquire (
	spinlock_t *lock
	)
{
#if defined (ARCH_IS_UNIPROCESSOR)
	// if we are on a uniprocessor system, much better to just disable interrupts
	arch_atomic_lock_up();
#else
	arch_atomic_lock_mp(lock);
#endif	
}

void
spinlock_release (
	spinlock_t *lock
	)
{
#if defined (ARCH_IS_UNIPROCESSOR)
	arch_atomic_free_up();
#else
	arch_atomic_free_mp(lock);
#endif
}
