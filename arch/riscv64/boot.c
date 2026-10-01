/*++
	Copyright 2026 Driftless Software Pvt. Ltd
	
	NEEV Firmware.

	Module Name:

		boot.c

	Description:

		RISCV64 bootloader.

	Author:

		Mayank Pathak (mpathak).
		27/09/2026
--*/

#include <stdint.h>
#include <stddef.h>
#include <riscv64/sbi.h>
#include <adefs.h>
#include <console/serial.h>
#include <osdef.h>

extern char __bss[], __bss_end[], __stack_top[];
static uint32_t g_timer_rate;

extern void trap_entry(void);

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid)
{
	register long a0 __asm__("a0") = arg0;
	register long a1 __asm__("a1") = arg1;
	register long a2 __asm__("a2") = arg2;
	register long a3 __asm__("a3") = arg3;
	register long a4 __asm__("a4") = arg4;
	register long a5 __asm__("a5") = arg5;
	register long a6 __asm__("a6") = fid;
	register long a7 __asm__("a7") = eid;

	__asm__ __volatile__("ecall"
						 : "=r"(a0), "=r"(a1)
						 : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
						   "r"(a6), "r"(a7)
						 : "memory");
	return (struct sbiret){.error = a0, .value = a1};
}

static inline uint64_t r_time(void)
{
	uint64_t t;
	__asm__ __volatile__("rdtime %0" : "=r"(t));
	return t;
}

static inline uint64_t r_sstatus(void)
{
	uint64_t x;
	__asm__ __volatile__("csrr %0, sstatus" : "=r"(x));
	return x;
}

static inline void w_sstatus(uint64_t x)
{
	__asm__ __volatile__("csrw sstatus, %0" :: "r"(x));
}

static inline void w_sie(uint64_t x)
{
	__asm__ __volatile__("csrw sie, %0" :: "r"(x));
}

static inline void w_stvec(uint64_t x)
{
	__asm__ __volatile__("csrw stvec, %0" :: "r"(x));
}

void putchar(char ch)
{
	sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}

static void sbi_set_timer(uint64_t stime_value)
{
	sbi_call((long)stime_value, 0, 0, 0, 0, 0, 0, 0);
}

void *memset(void *buf, char c, size_t n)
{
	uint8_t *p = (uint8_t *) buf;
	while (n--)
		*p++ = c;
	return buf;
}

void early_puts(const char *s)
{
	while (*s) {
		putchar(*s);
		s++;
	}
}

void early_putc(char c)
{
	putchar(c);
}

void early_puthex(uint64_t v)
{
	const char *d = "0123456789abcdef";
	early_puts("0x");
	for (int i = 60; i >= 0; i -= 4)
		early_putc(d[(v >> i) & 0xf]);
}

void timer_irq_handler(void)
{
	early_putc('.');

	sbi_set_timer(r_time() + g_timer_rate);
}

void trap_dispatch(void)
{
	uint64_t scause;
	__asm__ __volatile__("csrr %0, scause" : "=r"(scause));

	if ((scause & (1UL << 63)) && (scause & 0xff) == 5) {
		timer_irq_handler();
	}
}

uint32_t
clock_init (
	uint32_t ser_baud,
	uint32_t timer_rate
	)
{
	(void)ser_baud;

	uint32_t timer_hz = timer_rate * 10000000;

	g_timer_rate = timer_hz;

	w_stvec((uint64_t)trap_entry);

	sbi_set_timer(r_time() + timer_hz);

	w_sie(SIE_STIE);
	w_sstatus(r_sstatus() | SSTATUS_SIE);

	return STATUS_SUCCESS;
}

void virt_startup(uint32_t hart_id, void *dtb)
{
	memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

	firmware_info_t binfo;
	struct dtb_info info;
	struct console_ops ser_ops;

	early_puts("NEEV Booting...\n\n");

	early_puts("boot: Probing system memory\n");

	uint64_t mem_base;
	uint64_t mem_size;

	//
	// Parse device tree
	//

	if (!dtb_get_memory(dtb, &mem_base, &mem_size)) {
		early_puts("boot: Error probing memory\n");
		while(1);
	}

	if (!dtb_parse(dtb, &info)) {
		early_puts("boot: Failed to parse device tree\n");
		while(1);
	}

	//
	// Fill boot information
	//

	uint64_t con = info.nuarts ? info.uarts[0].base : 0;
	for (uint32_t i=0; i<info.nuarts; i++) {
		if (info.uarts[i].is_stdout) {
			con = info.uarts[i].base;
		}
	}

	ser_ops.putc = early_putc;
	ser_ops.getc = 0;

	binfo.fw_total_ram = mem_size;
	binfo.fw_ram_base = mem_base;
	binfo.fw_core_num = hart_id;
	binfo.fw_usable_ram_offset = 0x400000;
	binfo.fw_virt_offset = 0x0;
	binfo.fw_ser_base = con;
	binfo.fw_ser_ops = &ser_ops;
	binfo.fw_is_video = 0;
	binfo.fw_vid_info = 0;

	early_puts("boot: Loading kernel\n");
	neev_init(&binfo);

	for (;;);
}

__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void)
{
	__asm__ __volatile__(
		"mv t0, a0\n"
		"mv t1, a1\n"
		"mv sp, %[stack_top]\n"
		"mv a0, t0\n"
		"mv a1, t1\n"
		"j virt_startup\n"
		:
		: [stack_top] "r" (__stack_top)
		: "t0", "t1", "a0", "a1"
	);
}
