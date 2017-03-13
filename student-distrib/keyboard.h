#ifndef _KEYBOARD_H
#define _KEYBOARD_H

/* PIC Interrupt Line and IDT Vector Number */
#define KEYBOARD_PORT       0x60

/* magic numbers */
#define NUM_KEYS			60

/* Initialize the keyboard */
extern void init_keyboard();
/* Process interrupts */
extern void handle_keyboard();

#endif
