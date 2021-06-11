#ifndef IO_H
#define IO_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* I/O Assembly */
static inline void outb(uint16_t port, uint8_t val) {
	__asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void outw(uint16_t port, uint16_t val) {
	__asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}

static inline void outl(uint16_t port, uint32_t val) {
	__asm volatile ( "outl %0, %w1" : : "a"(val), "Nd"(port) );
}

static inline uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm volatile ( "inb %1, %0" 
					: "=a"(ret)
					: "Nd"(port) );
	return ret;
}

static inline uint16_t inw(uint16_t port) {
	uint16_t ret;
	__asm volatile ( "inw %1, %0" 
					: "=a"(ret)
					: "Nd"(port) );
	return ret;
}

static inline uint32_t inl(uint16_t port) {
	uint32_t ret;
	__asm volatile ( "inl %1, %0" 
					: "=a"(ret)
					: "Nd"(port) );
	return ret;
}

static inline void io_wait(void) {
	__asm volatile ( "jmp 1f\n\t"
					"1: jmp 2f\n\t"
					"2:" );
}

static inline void disable_interrupts() {
	__asm volatile ("cli");
}

static inline void enable_interrupts() {
	__asm volatile ("sti");
}

static inline void send_EOI() {
	outb(0x20, 0x20);
	outb(0xa0, 0x20);
}

#endif
