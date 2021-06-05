#ifndef IO_H
#define IO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* I/O Assembly */
static inline void outb(uint16_t port, uint8_t val) {
	asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void outw(uint16_t port, uint16_t val) {
	asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void outl(uint16_t port, uint32_t val) {
	asm volatile ( "outl %0, %w1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	asm volatile ( "inb %1, %0" 
					: "=a"(ret)
					: "Nd"(port) );
	return ret;
}

static inline uint16_t inw(uint16_t port) {
	uint16_t ret;
	asm volatile ( "inw %1, %0" 
					: "=a"(ret)
					: "Nd"(port) );
	return ret;
}

static inline uint32_t inl(uint16_t port) {
	uint32_t ret;
	asm volatile ( "inl %1, %0" 
					: "=a"(ret)
					: "Nd"(port) );
	return ret;
}

static inline void io_wait(void) {
	asm volatile ( "jmp 1f\n\t"
					"1: jmp 2f\n\t"
					"2:" );
}

static inline void disable_interrupts() {
	asm volatile ("cli");
}

static inline void enable_interrupts() {
	asm volatile ("sti");
}

static bool are_interrupts_enabled() {
	unsigned long flags;
	asm volatile ( "pushf\n\t"
					"pop %0"
					: "=g"(flags) );
	return flags & (1 << 9);
}


#endif
