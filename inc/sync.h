#ifndef OS_CONSYNC_DEF_H
#define OS_CONSYNC_DEF_H

#include <stdint.h>

// raw integer for now, maybe add debugging info or etc later
typedef uint32_t spinlock_t;

void
spinlock_acquire (
	spinlock_t *lock
	);

void
spinlock_release (
	spinlock_t *lock
	);

#endif
