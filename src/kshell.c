#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <tio.h>
#include <kshell.h>
#include <keyboard_io.h>
#include <kstdio.h>
#include <kstdlib.h>

extern char const *kb_keyset;
extern char const *kb_keyset_upper;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void handleE0Key(MappedKey key) {
	switch(key.mapped_code) {
		case KEY_ARROW_UP:
			tioShiftTermLineProtected(-1);				
			break;

		case KEY_ARROW_DOWN:
			tioShiftTermLineProtected(1);				
			break;

		case PAGE_UP:
			tioShiftTermLineProtected(-(TERM_HEIGHT / 2));				
			break;

		case PAGE_DOWN:
			tioShiftTermLineProtected(TERM_HEIGHT / 2);
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
