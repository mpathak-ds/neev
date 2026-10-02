/*++
	Copyright 2026 Driftless Software Pvt. Ltd
	
	NEEV Firmware.

	Module Name:

		qemu_ramfb.c

	Description:

		QEMU RamFB driver.

	Author:

		Mayank Pathak (mpathak).
		02/10/2026
--*/

#include <stdint.h>
#include <adefs.h>
#include <osdef.h>
#include <riscv64/sbi.h>

uint32_t bswap32(uint32_t x) {
	return ((x & 0x000000FFu) << 24) |
		   ((x & 0x0000FF00u) <<  8) |
		   ((x & 0x00FF0000u) >>  8) |
		   ((x & 0xFF000000u) >> 24);
}

uint64_t bswap64(uint64_t x) {
	return ((x & 0x00000000000000FFULL) << 56) |
		   ((x & 0x000000000000FF00ULL) << 40) |
		   ((x & 0x0000000000FF0000ULL) << 24) |
		   ((x & 0x00000000FF000000ULL) <<  8) |
		   ((x & 0x000000FF00000000ULL) >>  8) |
		   ((x & 0x0000FF0000000000ULL) >> 24) |
		   ((x & 0x00FF000000000000ULL) >> 40) |
		   ((x & 0xFF00000000000000ULL) >> 56);
}

uint16_t bswap16(uint16_t x) {
	return (uint16_t)((x << 8) | (x >> 8));
}

int memcmp_str(void *str1, void *str2, uint64_t count) {
	unsigned char *s1 = (unsigned char*)str1;
	unsigned char *s2 = (unsigned char*)str2;

	while (count-- > 0) {
		if (*s1++ != *s2++)
		return s1[-1] < s2[-1] ? -1 : 1;
	}
	return 0;
}

static void mmio_write_bsw64(uint64_t addr, uint64_t val) {
	volatile uint64_t *mmio_w = (volatile uint64_t*)addr;
	*mmio_w = bswap64(val);
}

static void mmio_write64(uint64_t addr, uint64_t val) {
	volatile uint64_t *mmio_w = (volatile uint64_t*)addr;
	*mmio_w = val;
}

static void mmio_write_bsw16(uint64_t addr, uint16_t val) {
	volatile uint16_t *mmio_w = (volatile uint16_t*)addr;
	*mmio_w = bswap16(val);
}

static void mmio_write16(uint64_t addr, uint16_t val) {
	volatile uint16_t *mmio_w = (volatile uint16_t*)addr;
	*mmio_w = val;
}


static uint64_t mmio_read_bsw64(uint64_t addr) {
	return bswap64(*((volatile uint64_t*)addr));
}

static uint64_t mmio_read64(uint64_t addr) {
	return *((volatile uint64_t*)addr);
}

static uint16_t mmio_read_bsw16(uint64_t addr) {
	return bswap16(*((volatile uint16_t*)addr));
}

static uint16_t mmio_read16(uint64_t addr) {
	return *((volatile uint16_t*)addr);
}

static uint16_t mmio_read_bsw32(uint64_t addr) {
	return bswap32(*((volatile uint32_t*)addr));
}

static uint16_t mmio_read32(uint64_t addr) {
	return *((volatile uint32_t*)addr);
}

static void qemu_cfg_dma_transfer(void *address, uint32_t length, uint32_t control) {
	qemu_cfg_dma_access access = { .address = bswap64((uint64_t)address), .length = bswap32(length), .control = bswap32(control) };

	if (length == 0) {
		return;
	}

	mmio_write_bsw64(BASE_ADDR_ADDR, (uint64_t)&access);

	while(bswap32(access.control) & ~QEMU_CFG_DMA_CTL_ERROR) {}
}

static void qemu_cfg_read(void *buf, int len) {
	qemu_cfg_dma_transfer(buf, len, QEMU_CFG_DMA_CTL_READ);
}

static void qemu_cfg_read_entry(void *buf, int e, int len) {
	uint32_t control = (e << 16) | QEMU_CFG_DMA_CTL_SELECT | QEMU_CFG_DMA_CTL_READ;
	qemu_cfg_dma_transfer(buf, len, control);
}

void qemu_cfg_write_entry(void *buf, uint32_t e, uint32_t len) {
	uint32_t control = (e << 16) | QEMU_CFG_DMA_CTL_SELECT | QEMU_CFG_DMA_CTL_WRITE;
	qemu_cfg_dma_transfer(buf, len, control);
}

int ramfb_cfg_find_file() {
	uint32_t count, e, select;
	qemu_cfg_read_entry(&count, QEMU_CFG_FILE_DIR, sizeof(count));
	count = bswap32(count);

	for (select = 0, e = 0; e < count; e++) {
		struct qemu_cfg_file qfile;
		qemu_cfg_read(&qfile, sizeof(qfile));
		if (memcmp_str(qfile.name, "etc/ramfb", 10) == 0)
			select = bswap16(qfile.select);
	}
	return select;
}

int ramfb_check_fw_cfg_dma() {
	mmio_write16(BASE_ADDR_SELECTOR, 0x0000);
	union fw_cfg_sig_read cfg_sig_read;
	cfg_sig_read.theInt = *((volatile uint32_t*)BASE_ADDR_DATA);
	if (cfg_sig_read.bytes[0] == 'Q' && cfg_sig_read.bytes[1] == 'E' && cfg_sig_read.bytes[2] == 'M' && cfg_sig_read.bytes[3] == 'U') {
		if (mmio_read_bsw64(BASE_ADDR_ADDR) == 0x51454d5520434647) {
			return 1;
		}
	}    return 0;
}

fb_info *ramfb_init(int w, int h)
{
	static fb_info fb;

	// todo to change this addr
	fb.fb_addr = 0x80500000;
    fb.fb_width = w;
    fb.fb_height = h;
    fb.fb_bpp = 4;
    fb.fb_stride = 4 * w;
    fb.fb_size = (4 * w) * h;

	uint32_t select = ramfb_cfg_find_file();
	if (!select) {
		early_puts("boot: Invalid video settings\n");
		while(1);
	}

	struct qemu_ramfb_cfg cfg = {
		.addr = bswap64(fb.fb_addr),
		.fourcc = bswap32(DRM_FORMAT_XRGB8888),
		.flags = bswap32(0),
		.width = bswap32(fb.fb_width),
		.height = bswap32(fb.fb_height),
		.stride = bswap32(fb.fb_stride),
	};

	qemu_cfg_write_entry(&cfg, select, sizeof(cfg));

	return &fb;
}