#include <stdint.h>
#include <io.h>
#include <kstdio.h>

typedef enum {
    COM1 = 0x3F8
} COM_Port;

typedef enum {
    COM_DATA = 0,
    COM_INTERRUPT_ENABLE   = 1,
    COM_BAUD_DIVISOR_LSB   = 0,
    COM_BAUD_DIVISOR_MSB   = 1,
    COM_INTERRUPT_AND_FIFO = 2,
    COM_LINE_CONTROL       = 3,
    COM_MODEM_CONTROL      = 4,
    COM_LINE_STATUS        = 5,
    COM_MODEM_STATUS       = 6,
    COM_SCRATCH            = 7
} COM_Register;

void setRegister(COM_Port port, COM_Register reg, uint8_t byte) {
    outb(port + reg, byte);
}

uint8_t readRegister(COM_Port port, COM_Register reg) {
    return inb(port + reg);
}

typedef struct {
    unsigned int data_bits : 2;
    unsigned int stop_bits : 1;
    unsigned int parity    : 3;
    unsigned int reserved  : 1;
    unsigned int dlab      : 1;
} Line_Control;

typedef struct {
    unsigned int data_available    : 1;
    unsigned int transmitter_empty : 1;
    unsigned int break_error       : 1;
    unsigned int status_change     : 1;
    unsigned int reserved          : 4;
} Interrupt_Enable;

void serialInit() {
    // Disable all UART-triggered interrupts
    Interrupt_Enable interrupt_enable;
    kmemset(&interrupt_enable, 0, 1);
    setRegister(COM1, COM_INTERRUPT_ENABLE, *((uint8_t*) &interrupt_enable));
    // Set UART options
    Line_Control line_control = {0};
    line_control.data_bits = 0b11; // 8-bits per character
    line_control.stop_bits =    0; // 1 stop bit
    line_control.parity    =    0; // No parity checking
    line_control.dlab      =    1; // About to set BAUD rate
    setRegister(COM1, COM_LINE_CONTROL, *((uint8_t*) &line_control));

    uint8_t baud_divisor_msb = 0x00, baud_divisor_lsb = 0x10;
    setRegister(COM1, COM_BAUD_DIVISOR_LSB, baud_divisor_lsb);
    setRegister(COM1, COM_BAUD_DIVISOR_MSB, baud_divisor_msb);

    line_control.dlab      = 0; // Done setting BAUD rate
    setRegister(COM1, COM_LINE_CONTROL, *((uint8_t*) &line_control));
}

void serialRead(uint8_t* buffer, size_t amt) {
    unsigned int i = 0;
    while (i < amt) {
        // Wait for data to be available
        bool ready;
        while (1) {
            ready = readRegister(COM1, COM_LINE_STATUS) & 0x1;
            if (ready) {
                break;
            }
        }
        // Read that data into buffer
        buffer[i] = readRegister(COM1, COM_DATA);
        i += 1;
    }
}
