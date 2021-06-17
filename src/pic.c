#define PIC1_CMD		0x20
#define PIC1_DATA		0x21
#define PIC2_CMD		0xA0
#define PIC2_DATA		0xA1
#define PIC_READ_IRR	0x0A // OCW3 irq ready next CMD read
#define PIC_READ_ISR	0x0B // OCw3 irq service next CMD read 

#include <stddef.h>
#include <stdint.h>
#include <io.h>

/* Helper func */
static uint16_t __pic_get_irq_reg(int ocw3)
{
    /* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
     * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
    outb(PIC1_CMD, ocw3);
    outb(PIC2_CMD, ocw3);
    return (inb(PIC2_CMD) << 8) | inb(PIC1_CMD);
}
 
/* Returns the combined value of the cascaded PICs irq request register */
uint16_t pic_get_irr(void)
{
    return __pic_get_irq_reg(PIC_READ_IRR);
}

/* Returns the combined value of the cascaded PICs in-service register */
uint16_t pic_get_isr(void)
{
    return __pic_get_irq_reg(PIC_READ_ISR);
}
