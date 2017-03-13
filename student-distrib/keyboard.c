#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "types.h"

static uint32_t scancode_map[60] = {
  // no caps and no shift
  '\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\0', '\0',
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\0', '\0', 'a', 's',
  'd', 'f', 'g', 'h', 'j', 'k', 'l' , ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',
  'b', 'n', 'm',',', '.', '/', '\0', '*', '\0', ' ', '\0'
};

/*
 * init_keyboard(void)
 *
 * DESCRIPTION: Initialize the keyboard on the PIC
 *
 * INPUTS: 	none
 * OUTPUTS: none
 *
 * SIDE EFFECTS: Enables the Keyboard PIC line
 *
 */
void init_keyboard() {
    cli();
    enable_irq(KEYBOARD_IRQ_LINE);
    sti();
}

/*
 * keyboard_handler()
 *
 * DESCRIPTION: Handles keyboard key press.
 *
 * INPUTS: none
 * OUTPUTS: none
 * SIDE EFFECTS: Prints the key pressed if valid
 *
 */
void
keyboard_handler()
{
    cli();

    /* Read from the keyboard's data buffer */
    uint32_t scancode = inb(KEYBOARD_PORT);

    if (scancode < NUM_KEYS) {
        putc(scancode_map[scancode]);
    }

    send_eoi(KEYBOARD_IRQ_LINE);

    sti();
}
