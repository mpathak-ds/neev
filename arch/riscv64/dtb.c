/*++
	Copyright 2026 Driftless Software Pvt. Ltd

	NEEV Firmware.

	Module Name:

		dtb.c

	Description:

		Device tree parser.

	Author:

		Alexander Bell (abell).
		28/09/2026
--*/

#include <stdint.h>
#include <riscv64/sbi.h>
#include <adefs.h>

static uint32_t rd32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
		   ((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
}

static inline uint64_t dtb_us_to_ticks(const struct dtb_info *info, uint64_t us)
{
	return (us * (info->timebase_freq / 1000000)) +
		   ((us * (info->timebase_freq % 1000000)) / 1000000);
}

static uint32_t be32(uint32_t x)
{
	return ((x & 0xff) << 24) | ((x & 0xff00) << 8) | ((x & 0xff0000) >> 8) | ((x >> 24) & 0xff);
}

static uint64_t rd_cells(const uint8_t *p, uint32_t cells)
{
	uint64_t v = 0;
	for (uint32_t i = 0; i < cells; i++)
		v = (v << 32) | rd32(p + 4 * i);
	return v;
}

static uint32_t align4(uint32_t x)
{
	return (x + 3) & ~3u;
}

static int str_eq(const char *a, const char *b)
{
	while (*a && *a == *b) { a++; b++; }
	return *a == *b;
}

static int str_starts(const char *s, const char *prefix)
{
	while (*prefix) {
		if (*s++ != *prefix++)
			return 0;
	}
	return 1;
}

static int prop_is(const uint8_t *v, uint32_t len, const char *s)
{
	return len > 0 && str_eq((const char *)v, s);
}

static int compat_has(const uint8_t *v, uint32_t len, const char *s)
{
	uint32_t i = 0;
	while (i < len) {
		const char *cur = (const char *)v + i;
		if (str_eq(cur, s))
			return 1;
		while (i < len && v[i]) i++;
		i++;
	}
	return 0;
}

static int is_uart_compat(const uint8_t *v, uint32_t len)
{
	static const char *const known[] = {
		"ns16550a", "ns16550", "ns8250",
		"snps,dw-apb-uart", "sifive,uart0",
	};
	for (uint32_t i = 0; i < sizeof(known) / sizeof(known[0]); i++)
		if (compat_has(v, len, known[i]))
			return 1;
	return 0;
}

static int stdout_matches(const char *path, uint64_t base)
{
	const char *at = 0;
	for (const char *s = path; *s && *s != ':'; s++)
		if (*s == '@')
			at = s + 1;
	if (!at)
		return 0;

	uint64_t v = 0;
	for (; *at && *at != ':' && *at != '/'; at++) {
		char c = *at;
		if (c >= '0' && c <= '9')      v = (v << 4) | (c - '0');
		else if (c >= 'a' && c <= 'f') v = (v << 4) | (c - 'a' + 10);
		else if (c >= 'A' && c <= 'F') v = (v << 4) | (c - 'A' + 10);
		else return 0;
	}
	return v == base;
}

static void finish_node(struct dtb_info *info, const struct node_frame *f, const struct node_frame *parent, int depth)
{
	uint32_t ac = parent->child_ac;
	uint32_t sc = parent->child_sc;

	if ((f->flags & NF_MEMORY) && !info->have_memory && f->reg &&
		f->reg_len >= 4 * (ac + sc)) {
		info->mem_base = rd_cells(f->reg, ac);
		info->mem_size = rd_cells(f->reg + 4 * ac, sc);
		info->have_memory = 1;
	}

	if (depth == 3 && str_eq(parent->name, "cpus") &&
		str_starts(f->name, "cpu@") && f->reg && f->reg_len >= 4 * ac) {
		if (info->nharts < DTB_MAX_HARTS) {
			struct dtb_hart *h = &info->harts[info->nharts++];
			h->id = rd_cells(f->reg, ac);
			h->available = !(f->flags & NF_STATUS_BAD);
			if (h->available)
				info->nharts_avail++;
		}
	}

	if ((f->flags & NF_UART) && !(f->flags & NF_STATUS_BAD) && f->reg &&
		f->reg_len >= 4 * (ac + sc) && info->nuarts < DTB_MAX_UARTS) {
		struct dtb_uart *u = &info->uarts[info->nuarts++];
		u->base= rd_cells(f->reg, ac);
		u->size = rd_cells(f->reg + 4 * ac, sc);
		u->clock_hz = f->clock_hz;
		u->reg_shift = f->reg_shift;
		u->is_stdout = 0;
	}
}

int dtb_parse(void *dtb, struct dtb_info *info)
{
	struct fdt_header *hdr = (struct fdt_header *)dtb;
	if (be32(hdr->magic) != FDT_MAGIC)
		return 0;

	const uint8_t *strings_blk = (const uint8_t *)dtb + be32(hdr->off_dt_strings);
	const uint8_t *p   = (const uint8_t *)dtb + be32(hdr->off_dt_struct);
	const uint8_t *end = p + be32(hdr->size_dt_struct);

	struct node_frame stack[DTB_MAX_DEPTH + 1];
	int depth = 0;

	uint8_t *z = (uint8_t *)info;
	for (uint32_t i = 0; i < sizeof(*info); i++) z[i] = 0;

	stack[0].name = "";
	stack[0].child_ac = 2;
	stack[0].child_sc = 1;

	while (p + 4 <= end) {
		uint32_t tok = rd32(p);
		p += 4;

		if (tok == FDT_BEGIN_NODE) {
			const char *name = (const char *)p;
			uint32_t namelen = 0;
			while (name[namelen]) namelen++;
			p += align4(namelen + 1);

			if (++depth > DTB_MAX_DEPTH)
				return 0;

			struct node_frame *f = &stack[depth];
			f->name      = name;
			f->reg       = 0;
			f->reg_len   = 0;
			f->child_ac  = 2;
			f->child_sc  = 1;
			f->flags     = 0;
			f->clock_hz  = 0;
			f->reg_shift = 0;

			if (str_starts(name, "memory"))
				f->flags |= NF_MEMORY;
		}
		else if (tok == FDT_END_NODE) {
			if (depth <= 0)
				return 0;
			finish_node(info, &stack[depth], &stack[depth - 1], depth);
			depth--;
		}
		else if (tok == FDT_PROP) {
			uint32_t len     = rd32(p);     p += 4;
			uint32_t nameoff = rd32(p);     p += 4;
			const char *pname = (const char *)(strings_blk + nameoff);
			const uint8_t *val = p;
			p += align4(len);

			if (depth < 1)
				continue;
			struct node_frame *f = &stack[depth];

			if (str_eq(pname, "reg")) {
				f->reg = val;
				f->reg_len = len;
			}
			else if (str_eq(pname, "#address-cells") && len == 4) {
				f->child_ac = rd32(val);
			}
			else if (str_eq(pname, "#size-cells") && len == 4) {
				f->child_sc = rd32(val);
			}
			else if (str_eq(pname, "device_type")) {
				if (prop_is(val, len, "memory"))
					f->flags |= NF_MEMORY;
			}
			else if (str_eq(pname, "compatible")) {
				if (is_uart_compat(val, len))
					f->flags |= NF_UART;
			}
			else if (str_eq(pname, "status")) {
				if (!prop_is(val, len, "okay") && !prop_is(val, len, "ok"))
					f->flags |= NF_STATUS_BAD;
			}
			else if (str_eq(pname, "clock-frequency") && len == 4) {
				f->clock_hz = rd32(val);
			}
			else if (str_eq(pname, "reg-shift") && len == 4) {
				f->reg_shift = rd32(val);
			}
			else if (str_eq(pname, "stdout-path") && depth == 2 &&
				 str_eq(f->name, "chosen")) {
				info->stdout_path = (const char *)val;
			}
			else if (str_eq(pname, "timebase-frequency") &&
				 (len == 4 || len == 8)) {
				int on_cpus = (depth == 2 && str_eq(f->name, "cpus"));
				int on_cpu  = (depth == 3 &&
						   str_eq(stack[depth - 1].name, "cpus") &&
						   str_starts(f->name, "cpu@"));

				if ((on_cpus || on_cpu) && !info->timebase_freq)
					info->timebase_freq = rd_cells(val, len / 4);
			}
		}
		else if (tok == FDT_NOP) {
		}
		else if (tok == FDT_END) {
			break;
		}
	}

	if (info->stdout_path)
		for (uint32_t i = 0; i < info->nuarts; i++)
			info->uarts[i].is_stdout =
				stdout_matches(info->stdout_path, info->uarts[i].base);

	return 1;
}

int dtb_get_memory(void *dtb, uint64_t *base_out, uint64_t *size_out)
{
	struct dtb_info info;
	if (!dtb_parse(dtb, &info) || !info.have_memory)
		return 0;
	*base_out = info.mem_base;
	*size_out = info.mem_size;
	return 1;
}
