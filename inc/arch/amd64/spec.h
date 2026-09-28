#ifndef AMD64_SPEC_ARCH_H
#define AMD64_SPEC_ARCH_H

#include <stdint.h>

#define COM1 0x3F8
#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_BSP 0x100
#define IA32_APIC_BASE_MSR_ENABLE 0x800

#define LAPIC_EOI 0x0B0
#define LAPIC_SVR 0x0F0
#define LAPIC_LVT_TMR 0x320
#define LAPIC_TMR_INIT 0x380
#define LAPIC_TMR_CUR 0x390
#define LAPIC_TMR_DIV 0x3E0

#define VEC_TIMER    0x20
#define VEC_SPURIOUS 0xFF

typedef struct
{
	uint16_t lim;
	uint64_t base;
} __attribute__((packed)) gdtr;

typedef struct
{
	uint16_t isr_low;
	uint16_t kcs;
	uint8_t ist;
	uint8_t attr;
	uint16_t isr_mid;
	uint32_t isr_high;
	uint32_t zero;	
} __attribute__((packed)) idt_entry;

typedef struct
{
	uint16_t lim;
	uint64_t base;
} __attribute__((packed)) idtr;

struct int_frame
{
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
	uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
	uint64_t vec, err;
	uint64_t rip, cs, rflags, rsp, ss;	
};

static inline void outb(uint16_t port, uint8_t v) {
    asm volatile ("outb %0, %1" : : "a"(v), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t v;
    asm volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static inline uint64_t rdmsr(uint32_t msr) {
	uint32_t low, high;
	asm volatile ("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
	return ((uint64_t)high<<32) | low;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
	uint32_t low = (uint32_t)val;
	uint32_t high = (uint32_t)(val>>32);
	asm volatile ("wrmsr" :: "a"(low), "d"(high), "c"(msr));
}

void amd64_init_gdt(void);
void amd64_init_idt(void);

int amd64_init_intctlr(void);
void amd64_init_timer_lapic(uint32_t hertz);
void amd64_eoi_lapic(void);

#endif
