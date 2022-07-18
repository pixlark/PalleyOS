// Serial Communication Interface
//
// This allows the operating system to communicate 
// to peripherals or to another system through serial ports.

#include <stdint.h>

void serialInit();

// Will block until `length` bytes are read from serial
void serialRead(uint8_t* buffer, unsigned int length);
