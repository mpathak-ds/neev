/*++
	Copyright 2026 Driftless Software Pvt. Ltd
	
	NEEV Firmware.

	Module Name:

		boot.c

	Description:

		AMD64 loader via limine.

	Author:

		Mayank Pathak (mpathak).
		28/09/2026
--*/

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <amd64/limine.h>
#include <amd64/spec.h>
#include <adefs.h>
#include <osdef.h>

__attribute__((used, section(".limine_requests")))
static volatile uint64_t base_revision[] = LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
	.id = LIMINE_MEMMAP_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
	.id = LIMINE_HHDM_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_mp_request mp_request = {
	.id = LIMINE_MP_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t requests_start_marker[2] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t requests_end_marker[2] = LIMINE_REQUESTS_END_MARKER;

void hcf()
{
	for (;;) {
		asm("hlt");
	}
}

void serial_init(uint32_t baud)
{
	uint16_t div = 115200 / baud;
	outb(COM1 + 1, 0x00);
	outb(COM1 + 3, 0x80);
	outb(COM1 + 0, div & 0xFF);
	outb(COM1 + 1, div >> 8);
	outb(COM1 + 3, 0x03);
	outb(COM1 + 2, 0xC7);
	outb(COM1 + 4, 0x0B);
}

static void serial_putc(char c)
{
	while (!(inb(COM1 + 5) & 0x20));
	outb(COM1, (uint8_t)c);
}

void early_puts(const char *s)
{
	for (; *s; s++) {
		if (*s == '\n') serial_putc('\r');
		serial_putc(*s);
	}
}

void early_puthex(uint64_t v)
{
	const char *d = "0123456789abcdef";
	early_puts("0x");
	for (int i = 60; i >= 0; i -= 4)
		serial_putc(d[(v >> i) & 0xf]);
}

uint32_t
clock_init (
	uint32_t ser_baud,
	uint32_t timer_rate
	)
{
	serial_init(ser_baud);

	int apic = amd64_init_intctlr();

	if (apic < 0) {
		return STATUS_FAILED_DEVICE_INIT;
	}

	early_puts("boot: setting timer\n");
	amd64_init_timer_lapic(timer_rate);

	return 0;
}

void amd_main(void)
{
	if (LIMINE_BASE_REVISION_SUPPORTED(base_revision) == false) {
		hcf();
	}

	firmware_info_t binfo;
	uint64_t mem_size = 0;
	uint64_t mem_base = 0x0;

	serial_init(115200);
	early_puts("NEEV Booting...\n\n");

	amd64_init_gdt();

	early_puts("boot: Probing system memory\n");

	if (memmap_request.response == NULL) {
		early_puts("boot: Failed to find memory map\n");
		hcf();
	}

	//
	// Probe and print whatever we find
	//

	struct limine_memmap_response *mm = memmap_request.response;
	uint64_t lowest = UINT64_MAX, highest = 0, usable = 0;
	
	for (uint64_t i = 0; i < mm->entry_count; i++) {
		struct limine_memmap_entry *e = mm->entries[i];
	
		early_puthex(e->base); early_puts(" len ");
		early_puthex(e->length); early_puts(" type ");
		early_puthex(e->type); early_puts("\n");
	
		if (e->type == LIMINE_MEMMAP_USABLE ||
		    e->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
			if (e->base < lowest) lowest = e->base;
			if (e->base + e->length > highest) highest = e->base + e->length;
			usable += e->length;
		}
	}

	mem_base = lowest;
	mem_size = usable;

	//
	// Find core id
	//

	if (mp_request.response == NULL) {
		early_puts("boot: Failed to find core ID\n");
		hcf();
	}

	early_puts("boot: Loading kernel\n");

	amd64_init_idt();

	binfo.fw_total_ram = mem_size;
	binfo.fw_ram_base = mem_base;
	binfo.fw_core_num = mp_request.response->bsp_lapic_id;
	binfo.fw_ser_base = COM1;
	
	neev_init(&binfo);

	hcf();
}
