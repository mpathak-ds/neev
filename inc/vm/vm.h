#ifndef OS_VMM_DEF_H
#define OS_VMM_DEF_H

#include <stdint.h>
#include <stddef.h>
#include <osdef.h>
#include <sync.h>

#define VM_PAGE_SIZE 4096

typedef uint64_t paddr_t;
typedef uint64_t vaddr_t;

// arch defines this
typedef struct page_table_t;

#endif
