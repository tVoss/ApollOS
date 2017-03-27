#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "types.h"

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
volatile uint8_t key_buffer[KEY_BUFFER_SIZE];
/* position in the key buffer */
volatile uint32_t key_buffer_pos = 0;

/* global variables for keyboard state */
static uint8_t enter_pressed = 0;
static uint8_t shift_pressed = 0;
static uint8_t capslock_state = 0;
static uint8_t ctrl_pressed = 0;
static uint8_t keys_state = 0;    // 0 = nothing pressed
                                  // 1 = shift pressed
                                  // 2 = capslock pressed
                                  // 3 = shift pressed and capslock pressed

//terminal_struct_t* terminal;

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

    send_eoi(KEYBOARD_IRQ_LINE);
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


      static int index = 0;

      // CHECKPOINT 2 TESTS
      switch (key_scancodes[keys_state][scancode]) {
          case '1':
            cp2_test_1();
            break;
          case '2':
            cp2_test_2("frame0.txt");
            break;
          case '3':
            cp2_test_3(index);
            index = (index + 1) % 17;
            break;
          default:
            break;
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
          // display key
          putc(key_scancodes[keys_state][scancode]);
        }
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
    int i;

    /* uncomment to show the terminal read and write are working */
    //terminal_read(0,key_buffer,key_buffer_pos+1);
    //terminal_write(0,key_buffer,key_buffer_pos+1);

    // clear key buffer
    for (i = 0; i < KEY_BUFFER_SIZE; i++){
      key_buffer[i] = '\0';
    }

    do_enter();

    key_buffer_pos = 0;
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

      do_backspace();
    }
}


/*
 * terminal_open()
 *
 * DESCRIPTION: provides access to file system
 *
 * INPUTS:       none
 * OUTPUTS:      none
 * SIDE EFFECTS: none
 *
 */
int32_t
terminal_open(){
  return 0;
}


/*
 * terminal_close()
 *
 * DESCRIPTION: closes specificed file descriptor
 *
 * INPUTS:       none
 * OUTPUTS:      none
 * SIDE EFFECTS: none
 *
 */
int32_t
terminal_close(){
  return 0;
}


/*
 * terminal_read()
 *
 * DESCRIPTION: reads data from the keyboard
 *
 * INPUTS:       fd - file descriptor
 *               buf - keyboard buffer to be read
 *               nbytes - number of bytes to read
 * OUTPUTS:      number of bytes read
 * SIDE EFFECTS: returns data from one line that has been
 *               terminated by pressing enter or as much fits
 *               on one line.
 *
 */
int32_t
terminal_read (int32_t fd, uint8_t* buf, int32_t nbytes) {
    int32_t bytes_read = 0;
    int32_t i = 0;
    // copy key_buffer
    while (key_buffer[i] != '\0' && i < nbytes){
      buf[i] = key_buffer[i];
      bytes_read++;
      i++;
    }
    // resest keyboard enter flag
    enter_pressed = 0;

    return bytes_read;
}

/*
 * terminal_write()
 *
 * DESCRIPTION: writes data to the terminal
 *
 * INPUTS:       fd - file descriptor
 *               buf - keyboard buffer to write
 *               nbytes - number of bytes to written
 * OUTPUTS:      number of bytes written
 * SIDE EFFECTS: data displayed to screen immediately.
 *
 */
int32_t terminal_write (int32_t fd, uint8_t* buf, int32_t nbytes) {
    int32_t i;
    int32_t bytes_written = 0;
    for (i = 0; i < nbytes; i++){
      if (buf[i] != '\0'){
        // write the char to the terminal
        putc(buf[i]);
        bytes_written++;
      } else {
        break;
      }
    }
    return bytes_written;
}
