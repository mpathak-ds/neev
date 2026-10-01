/*
 * NEEV Kernel
 * Path: vm/boot_alloc.c
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
#include <stddef.h>
#include <console/kd.h>
#include <osdef.h>
#include <vm/pm.h>

//
// Extremely simple allocator to get things going
//

uint64_t cur = 0;
uint64_t end = 0;

void
balloc_init (
	uint64_t base,
	uint64_t size
	)
{
	// align to boundary
	cur = ALIGN_UP(base, VM_FRAME_SIZE);
	end = base+size;
}

uint64_t
balloc (
	size_t bytes
	)
{
	uint64_t addr = ALIGN_UP(cur, VM_FRAME_SIZE);
	if (addr+bytes > end) panic(PANIC_OUT_OF_BALLOC, 0x00, "boot allocator out of memory for %lu bytes (addr=0x%lx, end=0x%lx)", bytes, addr, end);
	cur = addr+bytes;
	
	return addr;
}
