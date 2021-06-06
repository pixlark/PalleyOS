#include <io.h>
#include <tio.h>
#include <stddef.h>
#include <stdint.h>
#include <keyboard_io.h>

#define PS2	0x60
#define RESEND 0xFE
#define ECHO	0xEE
#define QUEUE_SIZE 256

uint8_t scan_code_queue[QUEUE_SIZE];
uint8_t queue_widx = 0;
uint8_t queue_ridx = 0;

/* Called from isr.s */
void keyboard_interrupt_handler(){
	uint8_t scan_code = inb(0x60);
	scan_code_queue[queue_widx++] = scan_code;
	send_EOI();
}

/* Returns keyboard response or 1 on error */
uint8_t ps2_send_sub_command(uint8_t command, int8_t subcommand) {
	outb(PS2, command);
	if(subcommand > 0)
		outb(PS2, subcommand);

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

uint8_t ps2_send_command(uint8_t command){
	return ps2_send_sub_command(command, -1);
}

bool ps2_set_keyset(uint8_t set_num) {
	return ps2_send_sub_command(0xF0, 2) != RESEND;
}

void setup_keyboard() {
	uint8_t echo = ps2_send_command(ECHO);
	if(echo != 1)
		term_write("Echo to keyboard recieved\n");		
	else {
		term_write("Keyboard not responding...Returning\n");
		return;
	}

	if(ps2_set_keyset(2))
		term_write("Keyset is 2\n");
	else 
		term_write("Could not set keyset\n");
}

void handle_scancode_queue() {
	while(true) {
		if(queue_ridx != queue_widx){
			disable_interrupts();

			term_write_uint32(scan_code_queue[queue_ridx], 16);
			queue_ridx++;
			enable_interrupts();		
		}
	}
}
