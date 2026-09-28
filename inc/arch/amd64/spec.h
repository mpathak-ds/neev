#ifndef AMD64_SPEC_ARCH_H
#define AMD64_SPEC_ARCH_H

#include <stdint.h>

#define COM1 0x3F8

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

void amd64_init_gdt(void);
void amd64_init_idt(void);

#endif
