#ifndef OS_PMM_DEF_H
#define OS_PMM_DEF_H

#include <stdint.h>
#include <stddef.h>
#include <osdef.h>

// bytes
#define VM_FRAME_SIZE 4096

void
balloc_init (
	uint64_t base,
	uint64_t size
	);

uint64_t
balloc (
	size_t bytes
	);

#endif
