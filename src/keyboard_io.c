#include <io.h>
#include <tio.h>
#include <stddef.h>
#include <stdint.h>
#include <keyboard_io.h>

#define PS2	0x60
#define RESEND 0xFE
#define ECHO	0xEE
#define QUEUE_SIZE 100

uint8_t scan_code_queue[QUEUE_SIZE];
uint8_t queue_index;

/* Called from isr.s */
void keyboard_interrupt_handler(){
	uint8_t scan_code = inb(0x60);
	if(queue_index < QUEUE_SIZE-1)
		scan_code_queue[queue_index++] = scan_code;
	send_EOI();
}

/* Returns keyboard response or 1 on error */
uint8_t send_command(uint8_t command) {
	int count = 0;
	int ret;
	while(count < 10){
		ret = inb(PS2);	
		if(ret != RESEND) break;
		count ++;
	}
	if(count >= 10) {
		return 1;
	}
	
	return ret;
}

void setup_keyboard() {
	uint8_t echo = send_command(ECHO);
	if(echo != 1)
		term_write("Echo to keyboard recieved\n");		
	else {
		term_write("Keyboard not responding...Returning\n");
		return;
	}

	send_command(0xF0);
	outb(PS2, 0);
	int set = inb(PS2);
	term_write_int(set, 16);

}

void handle_scancode_queue() {
	while(true) {
		if(queue_index > 0){
			
		}
	}
}
