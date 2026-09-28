/*++
	Copyright 2026 Driftless Software Pvt. Ltd
	
	NEEV Firmware.

	Module Name:

		ints.c

	Description:

		AMD64 interrupts setup.

	Author:

		Mayank Pathak (mpathak).
		28/09/2026
--*/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <amd64/spec.h>
#include <adefs.h>

__attribute__((aligned(0x10)))
static idt_entry g_idt[256];
static idtr g_idtr;
static bool vecs[256];

extern void *isr_stub_table[];

//generic handler
void amd64_ehandler(struct int_frame *f)
{
	early_puts("boot: exception triggered!\n");

	//
	// Display interrupt frame info
	//

	early_puts("vec=");
	early_puthex(f->vec);

	early_puts(" err=");
	early_puthex(f->err);

	early_puts(" rip=");
	early_puthex(f->rip);

	early_puts(" cs=");
	early_puthex(f->cs);
	early_puts("\n");

	if (f->vec == 14) {

		//
		// Page fault handler
		//

		uint64_t cr2;
		asm volatile ("mov %%cr2, %0" : "=r"(cr2));
		early_puts("boot: cr2=");
		early_puthex(cr2);
		early_puts("\n");
	}

	//
	// Hang
	//

	asm volatile ("cli; hlt");
}

void amd64_set_desc(uint8_t vec, void *isr, uint8_t flags)
{
	idt_entry *desc = &g_idt[vec];

	desc->isr_low = (uint64_t)isr & 0xFFFF;
	desc->kcs = 0x08;
	desc->ist = 0;
	desc->attr = flags;
	desc->isr_mid = ((uint64_t)isr>>16) & 0xFFFF;
	desc->isr_high = ((uint64_t)isr>>32) & 0xFFFFFFFF;
	desc->zero = 0;
}

void amd64_init_idt(void)
{
	g_idtr.base = (uintptr_t)&g_idt[0];
	g_idtr.lim = (uint16_t)sizeof(idt_entry) * 256 - 1;

	for (uint8_t vec=0; vec<32; vec++) {
		amd64_set_desc(vec, isr_stub_table[vec], 0x8E);
		vecs[vec] = true;
	}

	asm volatile ("lidt %0" : : "m"(g_idtr));

	//
	// Disable and remap before enabling ints
	//

	outb(0x21, 0xFF);
	outb(0xA1, 0xFF);
	
	asm volatile ("sti");
}
