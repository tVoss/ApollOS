#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "types.h"
#include "terminal.h"

#include "tests.h"

static uint8_t key_scancodes[KEY_STATES][NUM_KEYS] = {
  // no caps and no shift
  {'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\0', '\0',
   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\0', '\0', 'a', 's',
   'd', 'f', 'g', 'h', 'j', 'k', 'l' , ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',
   'b', 'n', 'm',',', '.', '/', '\0', '*', '\0', ' ', '\0'},
  // no caps and yes shift
  {'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\0', '\0',
   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\0', '\0', 'A', 'S',
   'D', 'F', 'G', 'H', 'J', 'K', 'L' , ':', '"', '~', '\0', '|', 'Z', 'X', 'C', 'V',
   'B', 'N', 'M', '<', '>', '?', '\0', '*', '\0', ' ', '\0'},
  // yes caps and no shift
  {'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\0', '\0',
   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\0', '\0', 'A', 'S',
   'D', 'F', 'G', 'H', 'J', 'K', 'L' , ';', '\'', '`', '\0', '\\', 'Z', 'X', 'C', 'V',
   'B', 'N', 'M', ',', '.', '/', '\0', '*', '\0', ' ', '\0'},
  // yes caps and yes shift
  {'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\0', '\0',
   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\0', '\0', 'a', 's',
   'd', 'f', 'g', 'h', 'j', 'k', 'l' , ':', '"', '~', '\0', '\\', 'z', 'x', 'c', 'v',
   'b', 'n', 'm', '<', '>', '?', '\0', '*', '\0', ' ', '\0'}
};


/* key buffer */
//volatile uint8_t *key_buffer;
/* position in the key buffer */
volatile uint32_t key_buffer_pos = 0;

/* global variables for keyboard state */
volatile uint8_t enter_pressed = 0;
static uint8_t shift_pressed = 0;
static uint8_t capslock_state = 0;
static uint8_t ctrl_pressed = 0;
static uint8_t alt_pressed = 0;
static uint8_t keys_state = 0;    // 0 = nothing pressed
                                  // 1 = shift pressed
                                  // 2 = capslock pressed
                                  // 3 = shift pressed and capslock pressed
int right_bound = 7;


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
    key_buffer_pos = 0;
    sti();
}

/*
 * keyboard_handler()
 *
 * DESCRIPTION: Handles keyboard input.
 *
 * INPUTS: none
 * OUTPUTS: none
 * SIDE EFFECTS: prints keyboard input to the screen.
 *
 */
void
keyboard_handler()
{
    cli();

    /* Read from the keyboard's data buffer */
    uint32_t scancode = inb(KEYBOARD_PORT);

    send_eoi(KEYBOARD_IRQ_LINE);

    if (scancode != 0xE0){
      switch (scancode) {
        // shift pressed
        case SHIFT_L:
        case SHIFT_R:
          shift_pressed = 1;
          keys_state = 1;
          if (capslock_state == 1)
            keys_state = 3;
          break;
        // shift released
        case RELEASE(SHIFT_L):
        case RELEASE(SHIFT_R):
          shift_pressed = 0;
          keys_state = 0;
          if (capslock_state == 1)
            keys_state = 2;
          break;
        // toggle capslock
        case CAPSLOCK:
          if (capslock_state == 0) {
            capslock_state = 1;
            keys_state = 2;
          } else {
            capslock_state = 0;
            keys_state = 0;
          }
          break;
        // ctrl pressed
        case CTRL:
          ctrl_pressed = 1;
          break;
        // ctrl released
        case RELEASE(CTRL):
          ctrl_pressed = 0;
          break;
        // alt pressed
        case ALT_L:
          alt_pressed = 1;
          break;
        //alt released
        case RELEASE(ALT_L):
          alt_pressed = 0;
          break;
        // switching terminals
        case F1_KEY:
          // switch to terminal 1
          if (alt_pressed == 1)
            switch_terminal(1);
          break;
        case F2_KEY:
          // switch to terminal 2
          if (alt_pressed == 1)
            switch_terminal(2);
          break;
        case F3_KEY:
          // switch to terminal 3
          if (alt_pressed == 1)
            switch_terminal(3);
          break;
        // left arrow key pressed
        case ARROW_LEFT:
          move_cursor_left();
          break;
        // right arrow key pressed
        case ARROW_RIGHT:
          move_cursor_right();
          break;
        // enter pressed
        case ENTER:
          handle_enter();
          break;
        // backspace pressed
        case BACKSPACE:
          handle_backpace();
          break;
        default:
          key_pressed_handler(scancode);
          break;
      }
    }

    sti();
}

/*
 * key_pressed_handler(uint8_t scancode)
 *
 * DESCRIPTION: Handles keyboard input.
 *
 * INPUTS: scancode - input from keyboard
 * OUTPUTS: none
 * SIDE EFFECTS: prints keyboard input to the screen.
 *               adjusts key buffer.
 *
 */
void
key_pressed_handler(uint8_t scancode){
    // check for valid scancode
    if (scancode >= NUM_KEYS)
      return;
    // if ctrl+l is pressed, clear terminal
    if (ctrl_pressed == 1) {
      if (key_scancodes[keys_state][scancode] == 'l'){
        clear();
        key_buffer_pos = 0;
      }
    }
    else if (scancode < NUM_KEYS) {
        if (key_buffer_pos < KEY_BUFFER_SIZE){
          // check for null scancode
          if (key_scancodes[keys_state][scancode] == NULL_SCANCODE)
            return;
          // update key buffer
          key_buffer[key_buffer_pos] = key_scancodes[keys_state][scancode];
          key_buffer_pos++;
          right_bound++;
          // display key
          putc(key_scancodes[keys_state][scancode]);
        }
    }
}

/*
 * clear_buffer()
 *
 * DESCRIPTION: clears the keyboard buffer
 *
 * INPUTS:      none
 * OUTPUTS:     none
 * SIDE EFFECTS: clears keyboard buffer
 *
*/
void clear_buffer() {

    key_buffer_pos = 0;
    right_bound = 7;
    int i;

    /* uncomment to show the terminal read and write are working */
    //terminal_read(0,key_buffer,key_buffer1_pos+1);
    //terminal_write(0,key_buffer,key_buffer1_pos+1);

    // clear key buffer
    for (i = 0; i < KEY_BUFFER_SIZE; i++){
      key_buffer[i] = '\0';
    }
}

/*
 * handle_enter()
 *
 * DESCRIPTION: Handles pressing enter
 *
 * INPUTS:  none
 * OUTPUTS: none
 * SIDE EFFECTS: goes to new line in terminal.
 *               resets key buffer.
 *
 */
void
handle_enter() {
    enter_pressed = 1;    // raise the enter_pressed flag
    right_bound = 7;
    do_enter();
}

/*
 * handle_backspace()
 *
 * DESCRIPTION: Handles pressing backspace
 *
 * INPUTS:  none
 * OUTPUTS: none
 * SIDE EFFECTS: removes previous entry in terminal.
 *               updates key buffer.
 *
 */
void
handle_backpace() {
    if (key_buffer_pos > 0){
      // remove backspaced char from buffer by putting null key at that index
      key_buffer_pos--;
      key_buffer[key_buffer_pos] = '\0';
      right_bound--;

      do_backspace();
    }
}

/*  
 * move_cursor_left()
 * 
 * DESCRIPTION: Moves the cursor left
 *
 * INPUTS:  none
 * OUTPUTS: none
 * SIDE EFFECTS: cursor is moved to the left one space
 *
*/
void move_cursor_left() {
  int x, y;
  x = get_screen_x();
  y = get_screen_y();
  if (x != 7){
    x--;
    set_screen_pos(x,y);
    update_cursor_loc(x,y);
    key_buffer_pos--;
  }
}

/*  
 * move_cursor_right()
 * 
 * DESCRIPTION: Moves the cursor right
 *
 * INPUTS:  none
 * OUTPUTS: none
 * SIDE EFFECTS: cursor is moved to the right one space
 *
*/
void move_cursor_right(){
  int x,y;
  x = get_screen_x();
  y = get_screen_y();
  if (x < right_bound){
    x++;
    set_screen_pos(x,y);
    update_cursor_loc(x,y);
    key_buffer_pos++;
  }
}
