#include <stddef.h>
#include <stdint.h>
#include <timer.h>
#include <tio.h>
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

	disable_interrupts();

	outb(0x43, 0);

	io_wait();

	count = inb(0x40);
	count |= inb(0x40)<<8;

	enable_interrupts();

	return count;
}

void set_PIT_count(uint16_t count) {
	disable_interrupts();

	outb(0x40, count&0xff);
	outb(0x40, (count&0xff00)>>8);

	enable_interrupts();
	return;
}

void init_timer() {
//	init_PIT(18);
	kprintf("IRQ0_ms: 0x%d\n", IRQ0_ms);
	kprintf("IRQ0_fraction_ms: %d\n", IRQ0_fraction_ms);
	kprintf("Reload Val: %d\n", PIT_reload_value);

	uint16_t pit_count = read_PIT_count();
	kprintf("PIT count: %d\n", pit_count);
}

// TODO: Update this to use a semaphore/exchange thingy
struct sleep_timer {
	void (*callback)();	
	uint32_t count_down;
} sleep_timers[20];
uint8_t sleep_timer_index = 0;

// Time is in intervals of 1ms
void add_sleep_timer(uint32_t time, void (*cb)()) {
	sleep_timers[sleep_timer_index].count_down = time;
	sleep_timers[sleep_timer_index].callback = cb;
}

uint16_t counter;
void TimerIRQ(void) { /* Called from PIT.s */
	uint8_t i;
	for (i = 0; i < 20; i++) {
		if(sleep_timers[i].count_down > 0) {
			sleep_timers[i].count_down--;
			if(sleep_timers[i].count_down == 0)
				sleep_timers[i].callback();			 // Probably a horrible idea
		}
	}
}
