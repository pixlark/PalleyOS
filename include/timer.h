/*
 *  This file pertains to the onboard legacy PIT timer
 *  It contains function declarations to intialize and set counters
 *  along with structures used with the PIT timer
 */

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

enum PITError {
    PITNoError = -1,
    PITCountersFull = -2,
    PITOutOfRange = -3,
};

struct PITResult {
    bool isError;
    union {
        uint32_t count;
        uint8_t counter_id;
        enum PITError error;
    };
};

struct PITCounter {
    bool active;
    uint32_t count;
};

typedef enum PITError PITError;
typedef struct PITResult PITResult;

typedef struct PITCounter PITCounter;
typedef uint32_t PITCounterCount;

/* ===== SLEEP ===== */
// Pauses execution for num_millis milliseconds
void pitSleep(uint32_t num_millis);

/* ===== COUNTERS ===== */
// Adds a counter to the pit counters
// On Success: Sets PITResult's counter_id for later reference
// On Error: Sets isError in the PITResult and sets the error
PITResult pitAddCounter();

// On Success: Deactivates the pit counter with index counter_id and
//             resets the count of the pit counter to 0
// On Error: Sets isError in the PITResult and set the error (most likely PITOutOfRange)
PITResult pitDeactivateCounter(uint8_t counter_id);

// On Success: Returns a PITResult with count set to
//             the current count of the pit counter with index counter_id
// On Error: Sets isError in the PITResult and set the error (most likely PITOutOfRange)
PITResult pitGetCounterCount(uint8_t counter_id);

// Sets the pit counter with index of counter_id count to 0
PITResult pitResetCounter(uint8_t counter_id);

/* ===== GETS ===== */
// Returns the PIT's frequency in Hertz
uint32_t pitGetFrequency();

/* ===== INITIALIZATION ===== */
// Initialized the PIT to a frequency of 1000Hz (each tick is 1ms)
void initPITTimer();

