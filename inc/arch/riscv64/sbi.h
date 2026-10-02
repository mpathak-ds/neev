#ifndef QEMU_VIRT_SBI_H
#define QEMU_VIRT_SBI_H

#include <stdint.h>

#define SSTATUS_SIE (1UL << 1)
#define SIE_STIE    (1UL << 5)

#define FDT_MAGIC       0xd00dfeed
#define FDT_BEGIN_NODE  0x1
#define FDT_END_NODE    0x2
#define FDT_PROP        0x3
#define FDT_NOP         0x4
#define FDT_END         0x9

#define DTB_MAX_DEPTH	16
#define DTB_MAX_HARTS	64
#define DTB_MAX_UARTS	8

#define NF_MEMORY (1u << 0)
#define NF_UART (1u << 1)
#define NF_STATUS_BAD (1u << 2)

#define QEMU_CFG_FILE_DIR 0x19

#define QEMU_CFG_DMA_CTL_ERROR 0x01
#define QEMU_CFG_DMA_CTL_READ 0x02
#define QEMU_CFG_DMA_CTL_SKIP 0x04
#define QEMU_CFG_DMA_CTL_SELECT 0x08
#define QEMU_CFG_DMA_CTL_WRITE 0x10

#define BASE_ADDR 0x10100000
#define BASE_ADDR_SELECTOR 0x10100008
#define BASE_ADDR_DATA 0x10100000
#define BASE_ADDR_ADDR 0x10100010

#define fourcc_code(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define DRM_FORMAT_XRGB8888 fourcc_code('X', 'R', '2', '4')

union fw_cfg_sig_read {
	uint32_t theInt;
	char bytes[sizeof(int)];
};

typedef struct {
	uint32_t control;
	uint32_t length;
	uint64_t address;
} __attribute__((__packed__)) qemu_cfg_dma_access;

struct __attribute__((__packed__)) qemu_ramfb_cfg {
	uint64_t addr;
	uint32_t fourcc;
	uint32_t flags;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
};

struct qemu_cfg_file {
	uint32_t size;
	uint16_t select;
	uint16_t reserved;
	char name[56];
};

typedef struct {
	uint64_t fb_addr;
	uint32_t fb_width;
	uint32_t fb_height;
	uint32_t fb_bpp;

	uint32_t fb_stride;
	uint32_t fb_size;
} fb_info;

struct node_frame {
	const char *name;
	const uint8_t *reg;
	uint32_t reg_len;
	uint32_t child_ac;
	uint32_t child_sc;
	uint32_t flags;
	uint32_t clock_hz;
	uint32_t reg_shift;
};

struct dtb_hart {
	uint64_t id;
	int available;
};

struct dtb_uart {
	uint64_t base;
	uint64_t size;
	uint32_t clock_hz;
	uint32_t reg_shift;
	int is_stdout;
};

struct dtb_info {
	int have_memory;
	uint64_t mem_base;
	uint64_t mem_size;

	uint32_t nharts;
	uint32_t nharts_avail;
	struct dtb_hart harts[DTB_MAX_HARTS];

	uint32_t nuarts;
	struct dtb_uart uarts[DTB_MAX_UARTS];

	uint64_t timebase_freq;

	const char *stdout_path;
};

struct fdt_header {
	uint32_t magic;
	uint32_t totalsize;
	uint32_t off_dt_struct;
	uint32_t off_dt_strings;
	uint32_t off_mem_rsvmap;
	uint32_t version;
	uint32_t last_comp_version;
	uint32_t boot_cpuid_phys;
	uint32_t size_dt_strings;
	uint32_t size_dt_struct;
};

struct sbiret {
	long error;
	long value;
};

int dtb_get_memory(void *dtb, uint64_t *base_out, uint64_t *size_out);
int dtb_parse(void *dtb, struct dtb_info *info);

int qemu_cfg_find_file();

#endif
