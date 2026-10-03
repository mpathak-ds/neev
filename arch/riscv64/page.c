/*++
	Copyright 2026 Driftless Software Pvt. Ltd
	
	NEEV Firmware.

	Module Name:

		page.c

	Description:

		RISCV64 paging.

	Author:

		Mayank Pathak (mpathak).
		03/10/2026
--*/

#include <stdint.h>
#include <adefs.h>
#include <osdef.h>
#include <vm/pm.h>
#include <vm/vm.h>

// identity mapped on boot
#define RV64_PHYS_TO_VIRT(pa) ((void*)(uint64_t)(pa))
#define RV64_VIRT_TO_PHYS(va) ((paddr_t)(uint64_t)(va))

#ifndef SATP_MODE_SV39
#define SATP_MODE_SV39 (8ULL << 60)
#endif

#ifndef MAKE_SATP_VAL
#define MAKE_SATP_VAL(pagetable_phys) (SATP_MODE_SV39 | ((uint64_t)(pagetable_phys) >> 12))
#endif

#define PT_ENTRIES 512
#define VA_VPN2_SHIFT 30
#define VA_VPN1_SHIFT 21
#define VA_VPN0_SHIFT 12
#define VA_INDEX_MASK 0x1FF
#define PTE_V (1ULL << 0)
#define PTE_R (1ULL << 1)
#define PTE_W (1ULL << 2)
#define PTE_X (1ULL << 3)
#define PTE_U (1ULL << 4)
#define PTE_G (1ULL << 5)
#define PTE_A (1ULL << 6)
#define PTE_D (1ULL << 7)

#define PAGE_KERNEL_EXEC (PTE_V | PTE_R | PTE_X | PTE_A | PTE_D)
#define PAGE_KERNEL_DATA (PTE_V | PTE_R | PTE_W | PTE_A | PTE_D)
#define PAGE_TABLE_NODE (PTE_V)

typedef uint64_t pte_t;
typedef pte_t arch_ptable_t[PT_ENTRIES];

__attribute__((aligned(4096)))
pte_t g_boot_page_table[PT_ENTRIES];

extern char __text_start[];
extern char __text_end[];
extern char __rodata_end[];

static inline pte_t pa_to_pte_ppn(uint64_t pa)
{
	return (pa>>12)<<10;
}

static inline paddr_t pte_to_pa(pte_t pte)
{
	return (pte>>10)<<12;
}

static inline void rv64_install_page_table(arch_ptable_t page_table)
{
	paddr_t pt_phys = RV64_VIRT_TO_PHYS(page_table);
	uint64_t satp = MAKE_SATP_VAL(pt_phys);

	asm volatile (
		"csrw satp, %0\n\t"
		"sfence.vma zero, zero"
		:
		: "r"(satp)
		: "memory"	
	);
}

pte_t *rv64_walk_table(arch_ptable_t page_table, vaddr_t va, uint8_t alloc)
{
	uint32_t vpn2 = (va >> VA_VPN2_SHIFT) & VA_INDEX_MASK;
	uint32_t vpn1 = (va >> VA_VPN1_SHIFT) & VA_INDEX_MASK;
	uint32_t vpn0 = (va >> VA_VPN0_SHIFT) & VA_INDEX_MASK;

	pte_t *pte2 = &page_table[vpn2];
	pte_t *l1_table;

	if (!(*pte2 & PTE_V)) {
		if (alloc) {
			paddr_t frame_pa = (paddr_t)falloc(VM_FRAME_SIZE);
			if (!frame_pa) {
				return NULL;
			}
			l1_table = (pte_t *)RV64_PHYS_TO_VIRT(frame_pa);
			memset(l1_table, 0, VM_PAGE_SIZE);

			*pte2 = pa_to_pte_ppn(frame_pa) | PAGE_TABLE_NODE;
		} else {
			return NULL;
		}
	} else {
		l1_table = (pte_t *)RV64_PHYS_TO_VIRT(pte_to_pa(*pte2)); 
	}

	pte_t *pte1 = &l1_table[vpn1];
	pte_t *l0_table;

	if (!(*pte1 & PTE_V)) {
		if (alloc) {
			paddr_t frame_pa = (paddr_t)falloc(VM_FRAME_SIZE);
			if (!frame_pa) {
				return NULL;
			}
			l0_table = (pte_t *)RV64_PHYS_TO_VIRT(frame_pa);
			memset(l0_table, 0, VM_PAGE_SIZE);

			*pte1 = pa_to_pte_ppn(frame_pa) | PAGE_TABLE_NODE;
		} else {
			return NULL;
		}
	} else {
		l0_table = (pte_t *)RV64_PHYS_TO_VIRT(pte_to_pa(*pte1)); 
	}

	pte_t *pte0 = &l0_table[vpn0];

	return pte0;
}

paddr_t rv64_virt_to_phys(arch_ptable_t page_table, vaddr_t va)
{
	pte_t *pte = rv64_walk_table(page_table, va, 0);
	if (!pte || !(*pte & PTE_V)) {
		return 0;
	}

	paddr_t addr = pte_to_pa(*pte);

	return addr | (va&0xFFF);
}

uint8_t rv64_map_page(arch_ptable_t page_table, vaddr_t va, paddr_t pa, uint64_t flags)
{
	if (va % VM_PAGE_SIZE != 0 || pa % VM_PAGE_SIZE != 0) {
		return 0;
	}

	pte_t *pte = rv64_walk_table(page_table, va, 1);
	if (!pte) {
		return 0;
	}

	*pte = pa_to_pte_ppn(pa) | PTE_V | flags;

	return 1;
}

paddr_t rv64_kvirt_to_phys(vaddr_t va)
{
	return rv64_virt_to_phys(g_boot_page_table, va);
}

uint8_t rv64_kmap_page(vaddr_t va, paddr_t pa, uint64_t flags)
{
	return rv64_map_page(g_boot_page_table, va, pa, flags);
}

void arch_init_mmu(pfirmware_info_t boot_info)
{
	memset(g_boot_page_table, 0, VM_PAGE_SIZE);

	//
	// Boot page tables are identity mapped for now
	//

	// read only mapping
	for (uint64_t addr=(uint64_t)__text_start; addr<(uint64_t)__rodata_end; addr+=VM_PAGE_SIZE) {
		if (!rv64_kmap_page(addr, addr, PAGE_KERNEL_EXEC)) {
			panic(PANIC_FAILED_LATE_INIT, 0x00, "mmu failed to map kernel ro page at 0x%lx", addr);
		}
	}

	// read write mapping
	for (uint64_t addr=ALIGN_UP((uint64_t)__rodata_end, VM_PAGE_SIZE); addr<(uint64_t)boot_info->fw_ram_base+boot_info->fw_usable_ram_offset+boot_info->fw_total_ram;
		addr+=VM_PAGE_SIZE)
	{
		if (!rv64_kmap_page(addr, addr, PAGE_KERNEL_DATA)) {
			panic(PANIC_FAILED_LATE_INIT, 0x00, "mmu failed to map kernel rw page at 0x%lx", addr);
		}
	}

	// map uart
	for (uint64_t addr=0x10000000; addr<0x10002000; addr+=VM_PAGE_SIZE) {
		if (!rv64_kmap_page(addr, addr, PAGE_KERNEL_DATA)) {
			panic(PANIC_FAILED_LATE_INIT, 0x00, "mmu failed to map mmio page at 0x%lx", addr);
		}
	}

	// install
	kprintf("init: installing page table\n");
	rv64_install_page_table(g_boot_page_table);
	kprintf("init: paging initialized at 0x%lx\n", (uint64_t)g_boot_page_table);
}
