#ifndef TIMER_H
#define TIMER_H

#include <stddef.h>
#include <stdint.h>

void add_sleep_timer(uint32_t time, void (*cb)());
void init_timer();

void set_PIT_count(uint16_t count);
uint16_t get_PIT_count(void);


#endif
