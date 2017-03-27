#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"

/* PIC Interrupt Line and IDT Vector Number */
#define KEYBOARD_PORT	0x60

/* magic numbers */
#define NUM_KEYS		60
#define KEY_STATES		4
#define KEY_BUFFER_SIZE 128
#define NULL_SCANCODE	'\0'

/* keyboard scancodes */
#define CTRL 			0x1D
#define ALT 			0x38
#define SHIFT_L 		0x2A
#define SHIFT_R 		0x36
#define CAPSLOCK 		0x3A

/* special keys */
#define ENTER	 		0x1C
#define BACKSPACE	 	0x0E

#define RELEASE(key)  	(key|0x80)  // key release code

/* The I/O ports */
#define FB_COMMAND_PORT         0x3D4
#define FB_DATA_PORT            0x3D5
/* The I/O port commands */
#define FB_HIGH_BYTE_COMMAND    14
#define FB_LOW_BYTE_COMMAND     15

/* Initialize the keyboard */
extern void init_keyboard();
/* Process interrupts */
extern void handle_keyboard();
/* Process key pressed */
extern void key_pressed_handler(uint8_t scancode);
/* print character with new line consideration*/
void putc_t(uint8_t c);
/* handles backspace pressed */
void handle_backpace();
/* handles enter pressed */
void handle_enter();
/* terminal driver system calls */
int32_t terminal_open();
int32_t terminal_close();
int32_t terminal_read (int32_t fd, uint8_t* buf, int32_t nbytes);
/* clear screen */
void clear_terminal();
/* update cursor location */
void update_cursor_loc(unsigned short pos);
/* shift the terminal one line up */
void scroll();

#endif
