#ifndef AMD64_SPEC_ARCH_H
#define AMD64_SPEC_ARCH_H

#include <stdint.h>

#define COM1 0x3F8

static inline void outb(uint16_t port, uint8_t v) {
    asm volatile ("outb %0, %1" : : "a"(v), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t v;
    asm volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

#endif
