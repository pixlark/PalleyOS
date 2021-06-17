#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <io.h>
#include <tio.h>
#include <keyboard_io.h>
#include <kstdio.h>

#define PS2			0x60
#define PS2_COMMAND 0x64
#define ACK			0xFA
#define RESEND 		0xFE
#define SCAN_CODE	0xF0
#define ECHO		0xEE
#define QUEUE_SIZE 	256

/* ===== MAPPING SCANCODES TO LOCATION ===== */

const char scancode_set1[] = {
	'?', '?', '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', '-', '=', '?', '?', 
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
	'o', 'p', '[', ']', '?', '?', 'a', 's',
	'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 
	'\'', '`', '?', '\\', 'z', 'x', 'c', 'v',
	'b', 'n', 'm', ',', '.', '/', '?', '*',
	'?', ' ', '?', '?', '?', '?', '?', '?',
	'?', '?', '?', '?', '?', '?', '?', '7',
	'8', '9', '-', '4', '5', '6', '+', '1',
	'2', '3', '0', '.', '?', '?', '?', '?',
	'?', '?'
};

const char uppercase_scancode_set1[] = {
	'?', '?', '!', '@', '#', '$', '%', '^',
	'&', '*', '(', ')', '_', '+', ' '/* backspace */, ' ', /* tab */
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
	'O', 'P', '{', '}', '?'/* enter */, '?' /* lctrl */, 'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', 
	'"', '~', '?' /* lshift */, '|', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M', '<', '>', '?', '?'/* rshift */, '*', /* keypad */
	'?'/* lalt */, ' ', '?' /* caps */, '?', '?', '?', '?', '?', /* ?'s are Fx */
	'?', '?', '?', '?', '?', '?'/*numlock*/, '?'/*scrollock*/, '7', /* numbers here are keypad */
	'8', '9', '-', '4', '5', '6', '+', '1', 
	'2','3', '0', '.', '?', '?', '?', '?' /* F11 */, 
	'?' /* F12 */
};

/* rowc_olum (n) */
/* 0xff means not mapped at all */
const uint8_t scancode_set1_to_mapped[] = {
	0xff, 0x00, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x40,
	0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x4b, 0x4c, 0x6c, 0xa0, 0x61, 0x62,
	0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
	0x6b, 0x20, 0x80, 0x4d, 0x81, 0x82, 0x83, 0x84,
    0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x33,
	0xa2, 0xa3, 0x60, 0x01, 0x02, 0x03, 0x04, 0x05, 
	0x06, 0x07, 0x08, 0x09, 0x0a, 0x31, 0x0e, 0x51, /* 7 on keypad */
	0x52, 0x53, 0x34, 0x6d, 0x6e, 0x6f, 0x8c, 0x8d,
	0x8e, 0x6b, 0x6c, 0xff, 0xff, 0xff, 0x0b, 0x0c
};


// TODO: Implement scancodes that start with 0xE0 byte
struct __attribute__((__packed__)) spec_keys_mapped {
	const uint8_t padding_0[0x35];
	const uint8_t keypad_fslash;		// (keypad) '/' 0x35
	const uint8_t padding_1[0x2];		// 0x36 0x37
	const uint8_t ralt; 				// 038
	const uint8_t padding_2[0xe];   	// 0x39 - 0x46
	const uint8_t home;   			// 0x47
	const uint8_t up_arrow;			// 0x48
	const uint8_t page_up;			// 0x49
	const uint8_t padding_3;			// 0x4a
	const uint8_t left_arrow; 		// 0x4b
	const uint8_t padding_4;			// 0x4c
	const uint8_t right_arrow; 		// 0x4d
	const uint8_t padding_5;			// 0x4e
	const uint8_t end;  				// 0x4f
	const uint8_t down_arrow; 		// 0x50
	const uint8_t page_down; 			// 0x51
	const uint8_t insert;				// 0x52
	const uint8_t del; 				// 0x53	
	const uint8_t padding_6[36];
}; 

const struct spec_keys_mapped special_keys_to_mapped = {
	 .keypad_fslash = 0x32,	// (keypad) '/'
	 .padding_1 = {0, 0},
	 .ralt = 0xa4, 	
	 .padding_2 = {0, 0, 0, 0, 0, 0},
	 .home = 0x2f,   
	 .up_arrow = 0x8c,	
	 .page_up = 0x30,	
	 .left_arrow = 0xa8, 	
	 .right_arrow = 0xaa, 	
	 .end = 0x4f,  	
	 .down_arrow = 0xa9, 
	 .page_down  = 0x50,	
	 .insert = 0x2e,
	 .del = 0x4e
};

uint8_t set1_to_mapped(uint8_t scancode1) {
	return scancode_set1_to_mapped[scancode1];
}

uint8_t special_to_mapped(uint8_t scancode1) {
	return *( ((uint8_t*)&special_keys_to_mapped) + scancode1);
}

/* Keys from all scancode sets will map to a byte by having the lowest
   5 bits correspont with the column # and the highest 3 bits corresponding
   with the row, giving a possible 256 keys (as expected with a byte)

   The top left of the keyboard is row 0, column 0
 */
static bool key_states[256] = {false};

static uint8_t scancode_queue[QUEUE_SIZE];
static uint8_t queue_widx = 0;
static uint8_t queue_ridx = 0;

static bool keyboard_initialized = false;
static int keyboard_state = KEYBOARD_STATE_NORMAL;

char const* kb_keyset;
char const* kb_keyset_upper;


/* ===== KEYBOARD INTERRUPT HANDLER ===== */
void keyboard_interrupt_handler(){
	uint8_t scan_code = inb(0x60);
	scancode_queue[queue_widx] = scan_code;
	queue_widx++;
	send_EOI();
}

/* ===== SENDING COMMANDS TO KEYBOARD ===== */

/* Returns keyboard response or 1 on error */
uint8_t ps2_send_sub_command(uint8_t command, int8_t subcommand) {
	disable_interrupts();
	outb(PS2, command);

	io_wait();
	if(subcommand >= 0)
		outb(PS2, subcommand);

	int count = 0;
	int ret;
	while(count < 10){
		ret = inb(PS2);	
		if(ret == ACK || ret == ECHO) break;
		count ++;
	}

	if(count >= 10) {
		return 1;
	}
	enable_interrupts();
	
	return ret;
}

/* Wrapper for ps2_send_sub_command */
uint8_t ps2_send_dev_command(uint8_t command){
	return ps2_send_sub_command(command, -1);
}

bool ps2_set_keyset(uint8_t set_num) {
	return ps2_send_sub_command(0xF0, set_num) != RESEND;
}

int8_t ps2_get_keyset() {
	int8_t scancode_set = -1;
	if(ps2_send_sub_command(SCAN_CODE, 0) == ACK){
		int ret;
		int count = 0;
		while(count < 10){
			ret = inb(PS2);	
			if(ret != RESEND) break;
			count ++;
		}
		scancode_set = ret;
	}
	return scancode_set;
}

// Send a command to ps2 command register 
uint8_t send_ps2_command(uint8_t comm, bool returns){
	outb(PS2_COMMAND, comm);
	io_wait();
	int num_tries = 0;
	uint8_t out_buff_status = 0;

	if(!returns) return 0xff;
	
	// Wait until status register says we can read
	while(num_tries++ < 10)
		if(out_buff_status = inb(0x64) & 1) break; 

	if(out_buff_status)
		return inb(PS2);

	return 0xff;
}

/* ===== INITIALISE PS/2 ===== */
void init_ps2() {

	// Disable Devices
	send_ps2_command(0xAD, false);
	send_ps2_command(0xA7, false);

	// Flush the Output Buffer
	inb(0x60);

	// Set controller configuration byte
	uint8_t ccb = send_ps2_command(0x20, true);
	ccb &= ~(1<<6); // translation
	ccb |= 3;		// enable interrupts
	outb(0x64, 0x60);
	outb(0x60, ccb);
	io_wait();

	bool dual_channeled = (ccb & (1<<5)) > 0;

	uint8_t test = 0;
	if((test = send_ps2_command(0xAA, true) != 0x55))
		kprintf("ERROR: ps2 controller test failed\n");
 
	if((test = send_ps2_command(0xAB, true) != 0x0))
		kprintf("ERROR: ps2 port 1 test failed: %d\n", test);

	if((test = send_ps2_command(0xA9, true) != 0x0))
		kprintf("ERROR: ps2 port 2 test failed: %d\n", test);

	// Re-enable devices
	send_ps2_command(0xAE, false);
	send_ps2_command(0xA8, false);


	if(ps2_send_dev_command(0xff) == 0xFD)
		kprintf("ERROR: Failed to reset ps2 device 1\n");
	
	outb(0x21, 0xfd);
	outb(0xa1, 0xff);
	
}


/* ===== INITIALISE KEYBOARD ===== */
void init_keyboard() {
	disable_interrupts();

	init_ps2();

	uint8_t echo = ps2_send_dev_command(ECHO);
	if(echo != 1)
		kprintf("Echo to keyboard recieved\n");		
	else {
		kprintf("Keyboard not responding...Returning\n");
		return;
	}

	kb_keyset = scancode_set1; 
	kb_keyset_upper = uppercase_scancode_set1;

	ps2_set_keyset(1);

	int keycode_set = ps2_get_keyset();
	if(keycode_set > 0){
		if(keycode_set == 0x43) keycode_set = 1;
		else if(keycode_set == 0x41) keycode_set = 2;
		else if(keycode_set == 0x3f) keycode_set = 3;

		kprintf("Scan code set: 0x%x\n", keycode_set);
	}else {
		kprintf("Error getting scan code set\n");	
	}

	keyboard_initialized = true;
	enable_interrupts();
	tio_enable_cursor();
}

/* ===== HANDLING INPUT  ===== */
bool kb_has_new_input() { return queue_ridx != queue_widx; }

MappedKey handle_e0_key() {
	MappedKey ret = {0};

	uint8_t curr_code = scancode_queue[queue_ridx++];
	bool being_pressed = (curr_code & (1<<7)) == 0;
	if(!being_pressed) curr_code ^= (1<<7);

	uint8_t mapped_code = special_to_mapped(curr_code);

	ret.mapped_code = mapped_code;
	ret.e0_key = true;
	ret.printable = false;
	ret.down_stroke = being_pressed;
	ret.scancode = curr_code;

	key_states[mapped_code] = being_pressed;

	keyboard_state = KEYBOARD_STATE_NORMAL;

	return ret;
}

MappedKey handle_unknown_key(uint8_t scancode, uint8_t mapped_code, bool being_pressed) {
	MappedKey ret = {0};
	ret.mapped_code = mapped_code;
	ret.printable = false;
	ret.down_stroke = being_pressed;
	ret.scancode = scancode;

	if(mapped_code != CAPS)
		key_states[mapped_code] = being_pressed;
	else if(being_pressed)
		key_states[CAPS] = !key_states[CAPS];
		

	return ret;
}
// Returns 0 on non-printable characters 
MappedKey kb_next_mapped_key() {
	MappedKey ret = {0};

	if(queue_ridx == queue_widx){
		enable_interrupts();
		while(queue_ridx == queue_widx);
		disable_interrupts();
	}

	uint8_t curr_code = scancode_queue[queue_ridx++];
	bool being_pressed = (curr_code & (1<<7)) == 0;
	
	if(curr_code == 0xE0) 
		return handle_e0_key();

	// Convert code from released to pressed
	if(!being_pressed) curr_code ^= (1<<7);

	uint8_t mapped_code = set1_to_mapped(curr_code);
	if(scancode_set1[curr_code] == '?')
		return handle_unknown_key(curr_code, mapped_code, being_pressed);
	
	key_states[mapped_code] = being_pressed;
	
	if(being_pressed) {
		if(key_states[L_SHIFT] || key_states[R_SHIFT] ||
		key_states[CAPS])
			ret.capitalized = true;
		else
			ret.capitalized = false;
	}

	ret.scancode = curr_code;
	ret.mapped_code = mapped_code;
	ret.printable = true;
	ret.down_stroke = being_pressed;

	uint32_t i;

	return ret;
}

char kb_next_char() {
	if(!keyboard_initialized) init_keyboard();
	MappedKey key = {0};
	while(key.printable == false || key.down_stroke == false) {
		if(queue_ridx != queue_widx) {
			disable_interrupts();
			key = kb_next_mapped_key();
			enable_interrupts();	
		}
	}

	char ret;
	if(key.capitalized) ret = uppercase_scancode_set1[key.scancode]; 
	else ret = scancode_set1[key.scancode];

	return ret;
}
