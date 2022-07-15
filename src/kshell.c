#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <tio.h>
#include <kshell.h>
#include <keyboard_io.h>
#include <kstdio.h>
#include <kstdlib.h>
#include "debug.h"

extern char const *kb_keyset;
extern char const *kb_keyset_upper;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void handleE0Key(MappedKey key) {
	switch(key.mapped_code) {
		case KEY_ARROW_UP:
        tio_shift_view(SHIFT_DIRECTION_UP, 1);
        break;
        
		case KEY_ARROW_DOWN:
        tio_shift_view(SHIFT_DIRECTION_DOWN, 1);
        break;
        
		case PAGE_UP: {
            tio_shift_view(SHIFT_DIRECTION_UP, (TERM_HEIGHT / 2));				
        }
        break;
        
		case PAGE_DOWN: {
            tio_shift_view(SHIFT_DIRECTION_DOWN, (TERM_HEIGHT / 2));
        }
        break;
		default:
        break;
	}	
}
#pragma GCC diagnostic push

void kShellStart() {
	kbInit();
	while(1) {
        kprintf("> ");
        char buffer[512];
        kmemset(buffer, 0, sizeof(buffer));
        kbGetLine(buffer);
        kprintf("\n%s\n", buffer);
    }
}
