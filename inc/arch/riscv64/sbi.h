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

#endif
