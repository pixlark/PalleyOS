/*
 *  This file pertains to the onboard legacy PIT timer
 *  It contains function definitions to intialize and set counters
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <timer.h>
#include <kstdio.h>
#include <kstdlib.h>
#include <io.h>
#include <idt.h>

#define PIT_CHANNEL_0_DATA    0x40
#define PIT_CHANNEL_1_DATA    0x41
#define PIT_CHANNEL_2_DATA    0x42
#define PIT_COMMAND_REG       0x43  // Write only

#define PIT_NUM_COUNTERS 50

static bool pit_initialized = false;
static int32_t frequency = 0;

static PITCounter pit_counters[PIT_NUM_COUNTERS] = {0}; // Perhaps make this dynamic 

static int32_t sleep_counter = 0;

void PITIRQ() {
    for(int i = 0; i < PIT_NUM_COUNTERS; i++){
        if(pit_counters[i].active)
            pit_counters[i].count ++;
    }

    if(sleep_counter > 0) {
        sleep_counter -= 1;
        //kprintf("sleep_counter: %d\n", sleep_counter);
    }

    outb(0x20, 0x20);
    outb(0xa0, 0x20);
}

static uint16_t readPITCount(void) {
	cli();
	uint16_t count = 0;

	outb(PIT_COMMAND_REG, 0);

	ioWait();

	count = inb(PIT_CHANNEL_0_DATA);
	count |= inb(PIT_CHANNEL_0_DATA)<<8;

	sti();
	return count;
}

static void setPITCount(uint16_t count) {
	cli();

	outb(PIT_CHANNEL_0_DATA, count&0xff);
	outb(PIT_CHANNEL_0_DATA, (count&0xff00)>>8);

	sti();
}

/* ===== SLEEP ===== */
// Pauses execution for num_millis milliseconds
void pitSleep(uint32_t num_millis) {
    sleep_counter = num_millis;
    while(true) {
        cli();
        if(sleep_counter == 0) break;
        sti();
        // We need to allow time for the Interrupt to be registered
        __asm__ volatile ("nop \n"
                           "nop\n"
                           "nop\n"
                           "nop\n"
                           "nop\n"
                           "nop\n"
                           "nop\n"
                           "nop\n");
    }
}

/* ===== COUNTERS ===== */
// Returns a PITError on error ( < 0)
// Returns the timer id for later access on success ( >= 0 )
PITResult pitAddCounter() {
    if(pit_initialized == false) initPITTimer();

    PITResult result;
    int i;
    for(i= 0; i < PIT_NUM_COUNTERS; i++) {
        if(pit_counters[i].active == false){
            pit_counters[i].active = true;
            result.isError = false;
            result.counter_id = i;
            return result;
        }
    }   
    result.isError = true;
    result.error = PITCountersFull;
    return result;
}

// Deactivates the pit counter with index counter_id
// Resets the count of the pit counter to 0
PITResult pitDeactivateCounter(uint8_t counter_id) {
    if(pit_initialized == false) initPITTimer();
    PITResult result;

    if(counter_id > PIT_NUM_COUNTERS) {
        result.isError = true;
        result.error = PITOutOfRange;
        return result;
    }
    pit_counters[counter_id].active = false;
    pit_counters[counter_id].count = 0; 

    result.isError = false;
    return result; 
}

// On Success, this returns a PITResult with count set to 
// the current count of the pit counter with index counter_id
PITResult pitGetCounterCount(uint8_t counter_id) {
    if(pit_initialized == false) initPITTimer();
    PITResult result;

    if(counter_id > PIT_NUM_COUNTERS) {
        result.isError = true;
        result.error = PITOutOfRange;
        return result;
    }

    result.isError = false;
    result.count = pit_counters[counter_id].count;
    return result;
}

// Sets the pit counter with index of counter_id count to 0
PITResult pitResetCounter(uint8_t counter_id) {
    if(pit_initialized == false) initPITTimer();
    PITResult result;
    if(counter_id > PIT_NUM_COUNTERS) {
        result.isError = true;
        result.error = PITOutOfRange;
        return result;
    }

    pit_counters[counter_id].count = 0;
    result.isError = false;
    return result;
}

/* ===== GETS ===== */
bool isPITInitialized() { return pit_initialized; }

uint32_t pitGetFrequency() { 
    if(!pit_initialized) initPITTimer();
    return frequency; 
}

/* ===== INITIALIZATION ===== */
extern void PITIRQHandler();
// Initializes the PIT timer to 1000Hz
void initPITTimer() {
	cli();
    if(pit_initialized) return;
    
    kmemset(pit_counters, 0, sizeof(PITCounter)*PIT_NUM_COUNTERS);

    // Set the PIT to a static 1000 Hz
	outb(PIT_COMMAND_REG, 0x34);
	outb(PIT_CHANNEL_0_DATA, 0xa9);
	outb(PIT_CHANNEL_0_DATA, 0x04);
	
	outb(PIT_COMMAND_REG, 0xe2);
    frequency = 1000;

    idtAddISR(0x20, &PITIRQHandler, 0, INTERRUPT_GATE_32);

    pit_initialized = true;
	sti();
}
