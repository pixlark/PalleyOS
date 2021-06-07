#include <io.h>
#include <tio.h>
#include <stddef.h>
#include <stdint.h>
#include <keyboard_io.h>
#include <kstdio.h>

#define PS2			0x60
#define ACK			0xFA
#define RESEND 		0xFE
#define SCAN_CODE	0xF0
#define ECHO		0xEE
#define QUEUE_SIZE 	256

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
	disable_interrupts();
	outb(PS2, command);
	io_wait();
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
	enable_interrupts();
	
	return ret;
}

uint8_t ps2_send_command(uint8_t command){
	return ps2_send_sub_command(command, -1);
}

bool ps2_set_keyset(uint8_t set_num) {
	return ps2_send_sub_command(0xF0, 2) != RESEND;
}

int8_t ps2_get_keyset() {
	int8_t scancode_set = -1;
	if(ps2_send_sub_command(SCAN_CODE, 0) == ACK)
		scancode_set = inb(PS2);
	return scancode_set;
}

void setup_keyboard() {
	disable_interrupts();
	outb(0x21, 0xfd);
	outb(0xa1, 0xff);
	uint8_t echo = ps2_send_command(ECHO);
	if(echo != 1)
		term_write("Echo to keyboard recieved\n");		
	else {
		term_write("Keyboard not responding...Returning\n");
		return;
	}

	int keycode_set = ps2_get_keyset();
	if(keycode_set > 0){
		kprintf("Scan code set: %d\n", keycode_set);
	}else {
		kprintf("Error getting scan code set\n");	
	}

	enable_interrupts();
	handle_scancode_queue();
}

void handle_scancode_queue() {
	while(true) {
		if(queue_ridx != queue_widx){
			disable_interrupts();

			kprintf("%x", scan_code_queue[queue_ridx]);
			queue_ridx++;
			enable_interrupts();		
		}
	}
}
