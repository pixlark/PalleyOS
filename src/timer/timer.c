#include <stddef.h>
#include <stdint.h>
#include <timer.h>
#include <kstdio.h>
#include <io.h>

#define COUNTOWN_COMPLETE 1

extern uint32_t system_timer_fraction_ms;
extern uint32_t system_timer_whole_ms;
extern uint16_t PIT_reload_value;
extern uint32_t IRQ0_frequency;
extern uint32_t IRQ0_fraction_ms;
extern uint32_t IRQ0_ms;

extern void init_PIT(uint32_t desired_freq);

uint16_t read_PIT_count(void) {
	uint16_t count = 0;

	cli();

	outb(0x43, 0);

	ioWait();

	count = inb(0x40);
	count |= inb(0x40)<<8;

	sti();

	return count;
}

void set_PIT_count(uint16_t count) {
	cli();

	outb(0x40, count&0xff);
	outb(0x40, (count&0xff00)>>8);

	sti();
	return;
}

void initPITTimer() {
	cli();
//	init_PIT(1000); // Externed in PIT.s
	outb(0x43, 0x34);
	outb(0x40, 0xa9);
	outb(0x40, 0x04);
	
	outb(0x43, 0xe2);
	ioWait();
	kprintf("PIT Status: 0x%x\n", inb(0x43));
	/*
	kprintf("IRQ0_ms: 0x%d\n", IRQ0_ms);
	kprintf("IRQ0_fraction_ms: %x\n", IRQ0_fraction_ms);
	kprintf("Reload Val: %d\n", PIT_reload_value);

	uint16_t pit_count = read_PIT_count();
	kprintf("PIT count: %x\n", pit_count);
	*/
	sti();
}
