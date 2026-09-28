/*++
	Copyright 2026 Driftless Software Pvt. Ltd
	
	NEEV Firmware.

	Module Name:

		apic.c

	Description:

		AMD64 interrupt controller.

	Author:

		Mayank Pathak (mpathak).
		28/09/2026
--*/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <amd64/spec.h>
#include <adefs.h>

static uint32_t g_lapic_ticks_per_ms;

static inline uint64_t amd64_read_x2apic(uint32_t reg)
{
	return rdmsr(0x800 + (reg>>4));
}

static inline void amd64_write_x2apic(uint32_t reg, uint64_t val)
{
	wrmsr(0x800 + (reg>>4), val);
}

void amd64_eoi_lapic(void)
{
	amd64_write_x2apic(LAPIC_EOI, 0);
}

static void amd64_wait_pit(void)
{
	//10ms
	uint16_t count = 11932;
	outb(0x61, (inb(0x61) & ~0x02) & ~0x01);
	outb(0x43, 0xB0);
	outb(0x42, count & 0xFF);
	outb(0x42, count>>8);
	outb(0x61, inb(0x61) | 0x01);
	while (!(inb(0x61) & 0x20));
}

bool amd64_check_apic(bool *has_x2apic)
{
	uint32_t eax, ebx, ecx, edx;

	asm volatile ("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));

	if (has_x2apic) {
		*has_x2apic = (ecx & (1<<21)) != 0;
	}

	return (edx & (1<<9)) != 0;
}

bool amd64_enable_apic(bool verify)
{
	//
	// Assumes caller has already checked if this hardware supports
	// APIC..
	//

	uint64_t base = rdmsr(IA32_APIC_BASE_MSR);

	//APIC
	base |= (1ULL<<11);
	//x2APIC
	base |= (1ULL<<10);

	wrmsr(IA32_APIC_BASE_MSR, base);

	if (verify) {
		uint64_t verif = rdmsr(IA32_APIC_BASE_MSR);

		if (!(verif & (1ULL<<11))) {
			early_puts("boot: failed to enable apic\n");
			return false;
		}

		if (!(verif & (1ULL<<10))) {
			early_puts("boot: failed to enable x2apic\n");
			return false;
		}
	}

	early_puts("boot: enabled x2apic\n");

	return true;
}

int amd64_init_intctlr(void)
{
	//
	// Check if this hardware is sufficient
	//

	bool has_x2apic = false;
	bool has_apic = amd64_check_apic(&has_x2apic);

	if (!has_apic) {
		early_puts("boot: no apic found\n");
		return -1;
	}

	if (!has_x2apic) {
		early_puts("boot: no x2apic available\n");
		return -2;
	}

	//
	// Enable hardware
	//

	if(!amd64_enable_apic(true)) {
		early_puts("boot: apic init failed\n");
		return -3;
	}

	//
	// Tests
	//

	uint32_t id = (uint32_t)amd64_read_x2apic(0x20);
	uint32_t ver = (uint32_t)amd64_read_x2apic(0x30);
	
	early_puts("boot: x2apic ID is ");
	early_puthex(id);
	early_puts(", version is ");
	early_puthex(ver);
	early_puts("\n");

	return 0;
}

void amd64_init_timer_lapic(uint32_t hertz)
{
	//
	// Enable spurious
	//

	amd64_write_x2apic(LAPIC_SVR, (1<<8) | VEC_SPURIOUS);

	//
	// Calibrate timer
	//

	amd64_write_x2apic(LAPIC_TMR_DIV, 0x03);
	amd64_write_x2apic(LAPIC_LVT_TMR, (1<<16) | VEC_TIMER);
	amd64_write_x2apic(LAPIC_TMR_INIT, 0xFFFFFFFF);
	amd64_wait_pit();
	uint32_t cur = (uint32_t)amd64_read_x2apic(LAPIC_TMR_CUR);
	amd64_write_x2apic(LAPIC_TMR_INIT, 0);

	g_lapic_ticks_per_ms = (0xFFFFFFFFu - cur) / 10;

	early_puts("boot: lapic ticks per ms = ");
	early_puthex(g_lapic_ticks_per_ms);
	early_puts("\n");

	//
	// Set to periodic mode
	//

	amd64_write_x2apic(LAPIC_LVT_TMR, (1<<17) | VEC_TIMER);
	amd64_write_x2apic(LAPIC_TMR_DIV, 0x03);
	amd64_write_x2apic(LAPIC_TMR_INIT, (g_lapic_ticks_per_ms*1000)/hertz);
}
