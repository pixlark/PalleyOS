#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <tio.h>
#include <io.h>
#include <keyboard_io.h>
#include <kstdio.h>

extern char const *kb_keyset;
extern char const *kb_keyset_upper;

static void handle_e0_key(MappedKey key) {
	switch(key.mapped_code) {
		case ARROW_UP:
			tio_shift_term_line(1);				
			break;

		case ARROW_DOWN:
			tio_shift_term_line(-1);				
			break;

		case PAGE_UP:
			tio_shift_term_line(TERM_HEIGHT / 2);				
			break;

		case PAGE_DOWN:
			tio_shift_term_line(-(TERM_HEIGHT / 2));
			break;
		default:
			break;
	}	
}

static void handle_non_printable_key(MappedKey key) {
	switch(key.mapped_code) {
		case(BACKSPACE):
			tio_dec_cursor();
			break;	
		case(ENTER):
			kprintf("\n");
			break;
		default:
			break;
	}
}

void terminal_proc_start() {
	init_keyboard();
	while(1) {
		MappedKey k = {0};
		
		while(!kb_has_new_input());
		
		disable_interrupts();
		k = kb_next_mapped_key();
		enable_interrupts();

		if(k.scancode == 0 || k.down_stroke == 0) continue;

		if(k.e0_key) handle_e0_key(k);
		else if(!k.printable && k.down_stroke){
			handle_non_printable_key(k);
		}
		else if(k.printable && k.down_stroke) {
			char to_print;
			if(k.capitalized)
				to_print = kb_keyset_upper[k.scancode];
			else
				to_print = kb_keyset[k.scancode];
			kprintf("%c", to_print);
		}
	}
}
