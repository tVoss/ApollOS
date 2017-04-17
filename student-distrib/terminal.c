#include "terminal.h"

#include "keyboard.h"
#include "lib.h"
#include "types.h"

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
int32_t terminal_open(const int8_t *filename) {
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
int32_t terminal_close(int32_t fd) {
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
int32_t terminal_read (int32_t fd, void *buf, int32_t nbytes) {

    while (!enter_pressed) {
        // Wait
    }

    enter_pressed = 0;

    int8_t *byte_buf = (int8_t *) buf;

    int32_t bytes_read = 0;
    int32_t i = 0;
    // copy key_buffer
    while (key_buffer[i] != '\0' && i < nbytes){
      byte_buf[i] = key_buffer[i];
      bytes_read++;
      i++;
    }

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
int32_t terminal_write(int32_t fd, const void *buf, int32_t nbytes) {

    const int8_t *byte_buf = (int8_t *) buf;

    int32_t i;
    int32_t bytes_written = 0;
    for (i = 0; i < nbytes; i++){
        if (byte_buf[i] != '\0'){
            // write the char to the terminal
            putc(byte_buf[i]);
            bytes_written++;
        } else {
            break;
        }
    }
    return bytes_written;
}
