#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "types.h"

#define VIDEO           0xB8000
#define NUM_COLS        80
#define NUM_ROWS        25
#define ATTRIB          0xA

static int screen_x;
static int screen_y;
static unsigned short cursor_pos;
static char* video_mem = (char *)VIDEO;


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
    clear_terminal(); 
    key_buffer_pos = 0;
    cursor_pos = (unsigned short)(NUM_COLS*screen_y + screen_x);
    update_cursor_loc(cursor_pos);
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
        clear_terminal();
        cursor_pos = (unsigned short)(NUM_COLS*screen_y + screen_x);
        update_cursor_loc(cursor_pos);
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
          // display key
          putc_t(key_scancodes[keys_state][scancode]);
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
    // check if terminal needs to be shifted up
    if (screen_y == NUM_ROWS - 1){
      scroll();
      cursor_pos = (unsigned short)(NUM_COLS*screen_y + screen_x);
      update_cursor_loc(cursor_pos);
      key_buffer_pos = 0;
    } else {
      screen_x = 0;
      screen_y++;
      cursor_pos = (unsigned short)(NUM_COLS*screen_y + screen_x);
      update_cursor_loc(cursor_pos);
      key_buffer_pos = 0;
    }
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
      // check if at beginning of line
      if (screen_x == 0 && screen_y > 0){
        screen_x = NUM_COLS - 2;
        screen_y--;
      } else {
        screen_x--;
      }

      *(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1)) = '\0';
      *(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB;

      cursor_pos = (unsigned short)(NUM_COLS*screen_y + screen_x);
      update_cursor_loc(cursor_pos);
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
        putc_t(buf[i]);
        bytes_written++;
      } else {
        break;
      }
    }
    return bytes_written;
}

/*
* void putc_t(uint8_t c);
*   Description: prints character with consideration of new line after 80 chars
*   Inputs: uint_8* c = character to print
*   Return Value: void
* Function: Output a character to the console 
*/
void
putc_t(uint8_t c)
{
    // check if reached the bottom of the terminal
    if ((screen_y == NUM_ROWS - 1) && (screen_x == NUM_COLS - 1)){
      scroll();
      cursor_pos = (unsigned short)(NUM_COLS*screen_y + screen_x);
      update_cursor_loc(cursor_pos);
      return;
    }
    // check if reached the end of the row
    if (screen_x == NUM_COLS - 1){
      screen_y++;
      screen_x = 0;
      *(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1)) = c;
      *(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB;
      screen_x++;
      screen_x %= NUM_COLS;
      screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
    } else {
      if(c == '\n' || c == '\r') {
        screen_y++;
        screen_x=0;
      } else {
        *(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1)) = c;
        *(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB;
        screen_x++;
        screen_x %= NUM_COLS;
        screen_y = (screen_y + (screen_x / NUM_COLS)) % NUM_ROWS;
      }
    }
    cursor_pos = (unsigned short)(NUM_COLS*screen_y + screen_x);
    update_cursor_loc(cursor_pos);
}

/*
* void clear_terminal(void)
*   Inputs: void
*   Return Value: none
*   Function: Clears video memory
*/
void
clear_terminal(void)
{
    int32_t i;
    for(i=0; i<NUM_ROWS*NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = '\0';
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
    }
    // reset x,y position
    screen_x = 0;
    screen_y = 0;
}

/*
* void update_cursor_loc(unsigned short pos)
*   Inputs: pos - x,y position of cursor
*   Return Value: none
*   Function: puts cursor in position pos.
*/
void update_cursor_loc(unsigned short pos){
    outb(FB_HIGH_BYTE_COMMAND, FB_COMMAND_PORT);
    outb(((pos >> 8) & 0x00FF), FB_DATA_PORT);
    outb(FB_LOW_BYTE_COMMAND, FB_COMMAND_PORT);
    outb(pos & 0x00FF, FB_DATA_PORT);
}


/*
* scroll()
*   Inputs:       none
*   Return Value: none
*   Function: shifts the terminal up one line
*/
void 
scroll() {
    int row;
    int col;
    int32_t current_line;
    int32_t scroll_line;
    // shift the rows up one
    for (row = 0; row < NUM_ROWS - 1; row++){
      for (col = 0; col < NUM_COLS; col++){
        current_line = NUM_COLS*(row+1) + col;
        scroll_line = NUM_COLS*row + col;
        *(uint8_t *)(video_mem+(scroll_line<<1)) = *(uint8_t *)(video_mem+(current_line<<1));
      }
    }
    // clear the bottom line
    for (col = 0; col < NUM_COLS; col++) {
      scroll_line = NUM_COLS*(NUM_ROWS-1) + col;
      *(uint8_t *)(video_mem + (scroll_line << 1)) = '\0';
    }
    // update the terminal location
    screen_x = 0;
    screen_y = NUM_ROWS - 1;
}

