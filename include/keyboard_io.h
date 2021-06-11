#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define L_SHIFT		0x80
#define R_SHIFT		0x8B
#define CAPS		0x60
#define BACKSPACE   0x2D
#define ENTER		0x6c

#define ARROW_UP	0x8c
#define ARROW_LEFT  0xa8	
#define ARROW_RIGHT 0xaa	
#define ARROW_DOWN	0xa9

#define PAGE_UP		0x30
#define PAGE_DOWN	0x50


#define KEYBOARD_STATE_NORMAL 	0
#define KEYBOARD_STATE_E0 		1

struct mapped_key {
	uint8_t mapped_code;
	uint8_t scancode;
	bool e0_key;
	bool printable;
	bool down_stroke;
	bool capitalized;
};

typedef struct mapped_key MappedKey;

void init_keyboard(void);
void keyboard_interrupt_handler(void);
void setup_keyboard(void);
void handle_scancode_queue(void);
struct mapped_key kb_next_mapped_key(void);
char kb_next_char();
bool kb_has_new_input();

