#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"
#include "terminal.h"

/* PIC Interrupt Line and IDT Vector Number */
#define KEYBOARD_PORT	0x60

/* magic numbers */
#define NUM_KEYS		60
#define KEY_STATES		4
#define KEY_BUFFER_SIZE 128
#define NULL_SCANCODE	'\0'

/* keyboard scancodes */
#define CTRL 			0x1D
#define ALT_L 			0x38
#define SHIFT_L 		0x2A
#define SHIFT_R 		0x36
#define CAPSLOCK 		0x3A
#define F1_KEY			0x3B
#define F2_KEY			0x3C
#define F3_KEY			0x3D

/* max number of terminals */
#define MAX_TERMINALS 	3

/* special keys */
#define ENTER	 		0x1C
#define BACKSPACE	 	0x0E

#define RELEASE(key)  	(key|0x80)  // key release code

/* key buffer */
//extern volatile uint8_t key_buffer[KEY_BUFFER_SIZE];
/* position in the key buffer */
extern volatile uint32_t key_buffer_pos;
/* if enter has been pressed */
extern volatile uint8_t enter_pressed;

/* Initialize the keyboard */
void init_keyboard();
/* Process interrupts */
void handle_keyboard();
/* Process key pressed */
void key_pressed_handler(uint8_t scancode);
/* print character with new line consideration*/
void putc_t(uint8_t c);
/* handles backspace pressed */
void handle_backpace();
/* handles enter pressed */
void handle_enter();
void clear_buffer();


#endif
