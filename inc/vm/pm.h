#ifndef OS_PMM_DEF_H
#define OS_PMM_DEF_H

#include <stdint.h>
#include <stddef.h>
#include <osdef.h>
#include <sync.h>

// bytes
#define VM_FRAME_SIZE 4096

struct frame_allocator
{
	uint8_t *bitmap;
	uint64_t total_frames;
	uint64_t free_frames;
	uint64_t resume;
	spinlock_t lock;
};

void
balloc_init (
	uint64_t base,
	uint64_t size
	);

uint64_t
balloc (
	size_t bytes
	);

uint64_t
balloc_freeze (
	void
	);

uint64_t
falloc_init (
	uint64_t top,
	uint64_t virt_offset
	);

uint64_t
falloc (
	uint64_t size
	);

void
ffree (
	uint64_t pa,
	uint64_t size
	);

void
falloc_mark_free (
	uint64_t base,
	uint64_t size
	);

#endif
