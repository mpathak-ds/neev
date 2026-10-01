/*
 * NEEV Kernel
 * Path: vm/pmm.c
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
#include <sync.h>

//
// This is a physical frame allocator using bitmap technique
//

struct frame_allocator g_alloc;

static inline int bm_test(uint64_t i)
{
	return (g_alloc.bitmap[i >> 3] >> (i & 7)) & 1;
}

static inline void bm_set(uint64_t i)
{
	g_alloc.bitmap[i >> 3] |= (uint8_t)(1u << (i & 7));
}

static inline void bm_clear(uint64_t i)
{
	g_alloc.bitmap[i >> 3] &= (uint8_t)~(1u << (i & 7));
}

uint64_t
falloc_init (
	uint64_t top,
	uint64_t virt_offset
	)

/*
	Function Description:

		This function initializes the physical frame allocator and sets all regions to used and gets
		rid of the bootstrap allocator.

	Function Parameters:

		top         - Highest usable physical address.
		virt_offset - Virtual offset for higher half mapping.

	Function Return:

		First unused address that bootstrap allocator has.

	Function Notes:

		This will permanently freeze the boot allocator.
*/

{
	uint64_t bitmap_bytes = 0;
	uint64_t bitmap_pa = 0x00;

	g_alloc.total_frames = top / VM_FRAME_SIZE;	
	bitmap_bytes = (g_alloc.total_frames+7) / 8;

	// allocate bitmap using bootalloc
	bitmap_pa = balloc(bitmap_bytes);

	// convert to address we can access
	g_alloc.bitmap = bitmap_pa+virt_offset;

	// every region is used on init

	for (uint64_t i=0; i<bitmap_bytes; i++) {
		g_alloc.bitmap[i] = 0xFF;
	}

	g_alloc.free_frames = 0;
	// frame 0 is reserved so start from 1
	g_alloc.resume = 1;

	spinlock_init(&g_alloc.lock);

	// freeze the bootstrap allocator
	return balloc_freeze();
}

void
falloc_mark_free (
	uint64_t base,
	uint64_t size
	)

/*
	Function Description:

		This function marks a certain range of physical memory free to use.

	Function Parameters:

		base - Starting physical address.
		size - Size of the range.

	Function Return:

		None.

	Function Notes:

		None.
*/

{
	uint64_t first = (base+VM_FRAME_SIZE-1) / VM_FRAME_SIZE;
	uint64_t last = (base+size) / VM_FRAME_SIZE;

	// frame 0 reserved
	if (first == 0) first = 1;
	if (last>g_alloc.total_frames) last = g_alloc.total_frames;

	spinlock_acquire(&g_alloc.lock);

	// 0 is free
	for (uint64_t i=first; i<last; i++) {
		if (bm_test(i)) {
			bm_clear(i);
			g_alloc.free_frames++;
		}
	}

	spinlock_release(&g_alloc.lock);
}

static int64_t bm_find_run(uint64_t start, uint64_t end, uint64_t n)
{
	uint64_t run = 0, first = 0;

	for (uint64_t i = start; i < end; i++) {
		// fast path
		// at byte boundary we skip 8 used frames at once
		if (run == 0 && (i & 7) == 0 && g_alloc.bitmap[i >> 3] == 0xFF) {
			i += 7;
			continue;
		}
		if (bm_test(i)) {
			run = 0;
		} else {
			if (run == 0)
				first = i;
			if (++run == n)
				return (int64_t)first;
		}
	}
	
	return -1;
}

uint64_t
falloc (
	uint64_t size
	)

/*
	Function Description:

		This function allocates page frames.

	Function Parameters:

		size - Total number of bytes.

	Function Return:

		Pointer to starting address, 0 if not found.

	Function Notes:

		This allocator will not zero memory before returning, so the caller must be aware and
		zero out the memory after if needed.
*/

{
	uint64_t n = (size+VM_FRAME_SIZE-1) / VM_FRAME_SIZE;
	uint64_t pa = 0;
	int64_t idx;

	if (!n) return 0;

	spinlock_acquire(&g_alloc.lock);

	if (n <= g_alloc.free_frames) {
		idx = bm_find_run(g_alloc.resume, g_alloc.total_frames, n);

		if (idx<0) {
			uint64_t end = g_alloc.resume+n;
			if (end>g_alloc.total_frames) end = g_alloc.total_frames;
			idx = bm_find_run(1, end, n);
		}

		if (idx>=0) {
			for (uint64_t i=0; i<n; i++) {
				bm_set((uint64_t)idx+i);

				g_alloc.free_frames -= n;
				g_alloc.resume = (uint64_t)idx+n;
				if (g_alloc.resume>=g_alloc.total_frames) {
					g_alloc.resume = 1;
				}

				pa = (uint64_t)idx*VM_FRAME_SIZE;
			}
		}
	}

	spinlock_release(&g_alloc.lock);
	return pa;
}

void
ffree (
	uint64_t pa,
	uint64_t size
	)

/*
	Function Description:

		This function frees page frames.

	Function Parameters:

		pa   - Starting physical address.
		size - Total number of bytes.

	Function Return:

		None.

	Function Notes:

		None.
*/

{
	uint64_t first = pa / VM_FRAME_SIZE;
	uint64_t n = (size+VM_FRAME_SIZE-1) / VM_FRAME_SIZE;

	spinlock_acquire(&g_alloc.lock);

	for (uint64_t i=first; i<first+n && i<g_alloc.total_frames; i++) {
		if (i != 0 && bm_test(i)) {
			bm_clear(i);
			g_alloc.free_frames++;
		}
	}

	if (first<g_alloc.resume && first != 0) {
		g_alloc.resume = first;
	}

	spinlock_release(&g_alloc.lock);
}
