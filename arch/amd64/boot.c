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
#include <console/video.h>

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

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request fb_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST_ID,
	.revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t requests_start_marker[2] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t requests_end_marker[2] = LIMINE_REQUESTS_END_MARKER;

static struct limine_framebuffer *g_framebuffer;

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

	return STATUS_SUCCESS;
}

static uint32_t fb_channel(uint8_t value, uint8_t mask_size, uint8_t mask_shift)
{
	uint64_t max = ((uint64_t)1 << mask_size) - 1;
	return (uint32_t)((value * max / 255) << mask_shift);
}

static uint32_t fb_pixel(struct limine_framebuffer *fb, uint8_t red, uint8_t green, uint8_t blue)
{
	return fb_channel(red, fb->red_mask_size, fb->red_mask_shift) | fb_channel(green, fb->green_mask_size, fb->green_mask_shift)
	| fb_channel(blue, fb->blue_mask_size, fb->blue_mask_shift);
}

static uint32_t fb_clr_pixel(struct limine_framebuffer *fb, uint32_t color)
{
	uint32_t result = fb_pixel(g_framebuffer, 255, 255, 255);
	
	switch (color)
	{
		case VIDEO_COLOR_BLACK:
			result = fb_pixel(g_framebuffer, 0, 0, 0);
			break;

		case VIDEO_COLOR_BLUE:
			result = fb_pixel(fb, 0, 0, 170);
			break;

		case VIDEO_COLOR_GREEN:
			result = fb_pixel(fb, 0, 170, 0);
			break;

		case VIDEO_COLOR_CYAN:
			result = fb_pixel(fb, 0, 170, 170);
			break;

		case VIDEO_COLOR_RED:
			result = fb_pixel(fb, 170, 0, 0);
			break;

		case VIDEO_COLOR_MAGENTA:
			result = fb_pixel(fb, 170, 0, 170);
			break;

		case VIDEO_COLOR_BROWN:
			result = fb_pixel(fb, 170, 85, 0);
			break;

		case VIDEO_COLOR_LIGHT_GREY:
			result = fb_pixel(fb, 170, 170, 170);
			break;

		case VIDEO_COLOR_DARK_GREY:
			result = fb_pixel(fb, 85, 85, 85);
			break;

		case VIDEO_COLOR_LIGHT_BLUE:
			result = fb_pixel(fb, 85, 85, 255);
			break;

		case VIDEO_COLOR_LIGHT_GREEN:
			result = fb_pixel(fb, 85, 255, 85);
			break;

		case VIDEO_COLOR_LIGHT_CYAN:
			result = fb_pixel(fb, 85, 255, 255);
			break;

		case VIDEO_COLOR_LIGHT_RED:
			result = fb_pixel(fb, 255, 85, 85);
			break;

		case VIDEO_COLOR_LIGHT_MAGENTA:
			result = fb_pixel(fb, 255, 85, 255);
			break;

		case VIDEO_COLOR_YELLOW:
			result = fb_pixel(fb, 255, 255, 85);
			break;

		case VIDEO_COLOR_WHITE:
			result = fb_pixel(fb, 255, 255, 255);
			break;
	
		default:
			break;
	}

	return result;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
	volatile uint32_t *fb_ptr = g_framebuffer->address;
	fb_ptr[y * (g_framebuffer->pitch / 4) + x] = fb_clr_pixel(g_framebuffer, color);
}

void fb_clrscr(uint32_t color)
{
	volatile uint32_t *fb_ptr = g_framebuffer->address;

	for (uint32_t x=0; x<g_framebuffer->width; x++) {
		for (uint32_t y=0; y<g_framebuffer->height; y++) {
			fb_ptr[y * (g_framebuffer->pitch / 4) + x] = fb_clr_pixel(g_framebuffer, color);
		}
	}
}

int fb_init()
{
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
	uint8_t fb_found = 1;
	struct console_ops serial_ops;
	struct video_console vid_info;
	struct limine_framebuffer *framebuffer;

	serial_init(115200);
	early_puts("NEEV Booting...\n\n");

	amd64_init_gdt();

	early_puts("boot: Probing system memory\n");

	if (memmap_request.response == NULL) {
		early_puts("boot: Failed to find memory map\n");
		hcf();
	}

	if (fb_request.response == NULL || fb_request.response->framebuffer_count < 1) {
		early_puts("boot: Found no display\n");
		fb_found = 0;
	}

	//
	// Probe and print whatever we find
	//

	struct limine_memmap_response *mm = memmap_request.response;
	uint64_t lowest = UINT64_MAX, highest = 0, usable = 0;
	uint64_t usable_ram_offset = 0;
	uint64_t max_usable_length = 0;
	
	for (uint64_t i = 0; i < mm->entry_count; i++) {
		struct limine_memmap_entry *e = mm->entries[i];
	
		early_puthex(e->base); early_puts(" len ");
		early_puthex(e->length); early_puts(" type ");
		early_puthex(e->type); early_puts("\n");

		if (e->type == LIMINE_MEMMAP_USABLE) {
			if (e->length > max_usable_length) {
				max_usable_length = e->length;
				usable_ram_offset = e->base;
			}
		}
		
		if (e->type == LIMINE_MEMMAP_USABLE ||
		    e->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE) {
			if (e->base < lowest) lowest = e->base;
			if (e->base + e->length > highest) highest = e->base + e->length;
			usable += e->length;
		}
	}

	mem_base = lowest;
	mem_size = usable;

	uint64_t hhdm_offset = 0;
	if (hhdm_request.response != NULL) {
		hhdm_offset = hhdm_request.response->offset;
	}

	//
	// Find core id
	//

	if (mp_request.response == NULL) {
		early_puts("boot: Failed to find core ID\n");
		hcf();
	}

	early_puts("boot: Loading kernel\n");

	amd64_init_idt();

	//
	// Probe display
	//

	if (fb_found) {
		framebuffer = fb_request.response->framebuffers[0];

		if (framebuffer->memory_model != LIMINE_FRAMEBUFFER_RGB || framebuffer->bpp != 32) {
			early_puts("boot: Unsupported display model\n");
			hcf();
		}

		g_framebuffer = framebuffer;

		vid_info.resolution_width = framebuffer->width;
		vid_info.resolution_height = framebuffer->height;
		vid_info.resolution_bpp = framebuffer->bpp;

		vid_info.initialize_screen = fb_init;
		vid_info.put_pixel = fb_put_pixel;
		vid_info.clear_screen = fb_clrscr;
	}

	serial_ops.putc = serial_putc;
	serial_ops.getc = NULL;

	binfo.fw_total_ram = mem_size;
	binfo.fw_ram_base = mem_base;
	binfo.fw_core_num = mp_request.response->bsp_lapic_id;
	binfo.fw_usable_ram_offset = usable_ram_offset;
	binfo.fw_virt_offset = hhdm_offset;
	binfo.fw_ser_base = COM1;
	binfo.fw_ser_ops = &serial_ops;
	binfo.fw_is_video = fb_found;
	binfo.fw_vid_info = &vid_info;
	
	neev_init(&binfo);

	hcf();
}
