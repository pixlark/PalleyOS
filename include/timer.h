#ifndef TIMER_H
#define TIMER_H

#include <stddef.h>
#include <stdint.h>

void add_sleep_timer(uint32_t time, void (*cb)());
void init_PIT_timer();

void set_PIT_count(uint16_t count);
uint16_t read_PIT_count(void);

// Duration in ms, located in src/timer/sleep.s
extern void sleep(uint32_t duration);

#endif
