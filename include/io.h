#ifndef IO_H
#define IO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* I/O Assembly */
static inline void outb(uint16_t port, uint8_t val) {
	__asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void outw(uint16_t port, uint16_t val) {
	__asm__ volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void outl(uint16_t port, uint32_t val) {
	__asm__ volatile ( "outl %0, %w1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile ( "inb %1, %0" 
					: "=a"(ret)
					: "Nd"(port) );
	return ret;
}

static inline uint16_t inw(uint16_t port) {
	uint16_t ret;
	__asm__ volatile ( "inw %1, %0" 
					: "=a"(ret)
					: "Nd"(port) );
	return ret;
}

static inline uint32_t inl(uint16_t port) {
	uint32_t ret;
	__asm__ volatile ( "inl %1, %0" 
					: "=a"(ret)
					: "Nd"(port) );
	return ret;
}

static inline void ioWait(void) {
	__asm__ volatile ( "jmp 1f\n\t"
					"1: jmp 2f\n\t"
					"2:" );
}

static inline void cli() {
	__asm__ volatile ("cli");
}

static inline void sti() {
	__asm__ volatile ("sti");
}

static inline void sendEndOfInterrupt() {
	outb(0x20, 0x20);
	outb(0xa0, 0x20);
}

#endif
