#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define KEY_L_SHIFT		0x80
#define KEY_R_SHIFT		0x8B
#define KEY_CAPS		0x60
#define KEY_BACKSPACE   0x2D
#define KEY_ENTER		0x6c

#define KEY_ARROW_UP	0x8c
#define KEY_ARROW_LEFT  0xa8	
#define KEY_ARROW_RIGHT 0xaa	
#define KEY_ARROW_DOWN	0xa9

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

void kbInit(void);
void keyboard_interrupt_handler(void);
void kbSetupKeyboard(void);
void handle_scancode_queue(void);

bool kb_has_new_input();

void kbGetLine(char* buffer);
char kbGetChar();
