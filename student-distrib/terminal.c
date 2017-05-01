#include "terminal.h"
#include "keyboard.h"
#include "lib.h"
#include "types.h"
#include "paging.h"
#include "syscalls.h"
#include "x86_desc.h"

/* global variables */
volatile terminal_t terminal[MAX_TERMINALS];
volatile int term_num;
volatile int term_cur;      // keep track of current terminal
volatile uint8_t *key_buffer;

uint32_t esp_temp;
uint32_t ebp_temp;

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
 * init_terminals()
 *
 * DESCRIPTION: intilaizes the three terminals and starts the
 *              first terminal by default.
 *
 * INPUTS:      none
 * OUTPUTS:     none
 * SIDE EFFECTS: starts first terminal, executes shell
 *
*/
void init_terminals () {
    uint32_t j;
    int32_t i;
    for (i = 0; i < MAX_TERMINALS ; i++){
        terminal[i].num = i+1;
        terminal[i].key_buffer_pos = 0;
        terminal[i].pos_x = 0;
        terminal[i].pos_y = 0;
        terminal[i].esp = 0;
        terminal[i].ebp = 0;
        terminal[i].num_processes = 0;
        terminal[i].init = 0;
        for (j = 0; j < KEY_BUFFER_SIZE; j++){
            terminal[i].key_buffer[j] = '\0';
        }
    }
    // map terminal to memory. each terminal has it's own memory
    remapToPage(TERM_MEM, TERM_1_MEM, 1);
    remapToPage(TERM_MEM, TERM_2_MEM, 2);
    remapToPage(TERM_MEM, TERM_3_MEM, 3);
    terminal[0].vid_mem = (uint8_t *)TERM_1_MEM;
    terminal[1].vid_mem = (uint8_t *)TERM_2_MEM;
    terminal[2].vid_mem = (uint8_t *)TERM_3_MEM;
    // clear video memory and set terminal color
    for(i = 0; i < NUM_ROWS*NUM_COLS; i++) {
        *(uint8_t *)(TERM_1_MEM + (i << 1)) = ' ';
        *(uint8_t *)(TERM_1_MEM + (i << 1) + 1) = ATTRIB_B;
        *(uint8_t *)(TERM_2_MEM + (i << 1)) = ' ';
        *(uint8_t *)(TERM_2_MEM + (i << 1) + 1) = ATTRIB_G;
        *(uint8_t *)(TERM_3_MEM + (i << 1)) = ' ';
        *(uint8_t *)(TERM_3_MEM + (i << 1) + 1) = ATTRIB_Y;
    }
    // start first terminals
    term_cur = 1;
    terminal_start(1);
}

int32_t switch_terminal(int term) {
    cli();
    // check if this is current terminal
    if (term == term_cur){
        sti();
        return 0;
    }
    
    if (terminal[term-1].init == 0){
        if (!can_execute()) {
            printf("\nPlease close processes before opening another teminal\n391OS> ");
            sti();
            return 0;
        }
    }

    // Always save
    terminal_save(term_cur);

    // Update current term
    term_cur = term;

    // Load or start depending on status
    if (terminal[term_cur - 1].init == 0) {
        terminal_start(term_cur);
    } else {
        terminal_load(term_cur);
    }

    sti();
    return 0;
}


int32_t terminal_start(int term)
{
    terminal[term-1].init = 1;
    key_buffer = terminal[term-1].key_buffer;
    key_buffer_pos = terminal[term-1].key_buffer_pos;
    set_screen_pos(terminal[term-1].pos_x, terminal[term-1].pos_y);
    update_cursor_loc(get_screen_x(),get_screen_y());
    memcpy((uint8_t *)VIDEO, (uint8_t *)terminal[term-1].vid_mem, 2*NUM_ROWS*NUM_COLS);
    printf("    _      ____     ___    _       _        ___             ___    ____  \n");
    printf("   / \\    |  _ \\   / _ \\  | |     | |      / _ \\           / _ \\  / ___| \n");
    printf("  / _ \\   | |_) | | | | | | |     | |     | | | |         | | | | \\___ \\ \n");
    printf(" / ___ \\  |  __/  | |_| | | |___  | |___  | |_| |         | |_| |  ___) |\n");
    printf("/_/   \\_\\ |_|      \\___/  |_____| |_____|  \\___/   _____   \\___/  |____/ \n");
    printf("                                                  |_____|                \n");
    
    execute("shell");
    return 0;
}


int32_t terminal_save(int term){
    asm volatile ("movl %%esp,%0 \n\t"					//store current esp and ebp in temporary variables to be put in the current terminal structure
          "movl %%ebp,%1 \n\t"
          : "=r"(esp_temp), "=r"(ebp_temp)
    );
    terminal[term-1].esp = esp_temp;
    terminal[term-1].ebp = ebp_temp;
    terminal[term-1].key_buffer_pos = key_buffer_pos;
    terminal[term-1].pos_x = get_screen_x();
    terminal[term-1].pos_y = get_screen_y();
    memcpy((uint8_t *)terminal[term-1].vid_mem, (uint8_t *)VIDEO, 2*NUM_ROWS*NUM_COLS);
    return 0;
}


int32_t terminal_load(int term){
    esp_temp = terminal[term-1].esp;
    ebp_temp = terminal[term-1].ebp;

    tss.ss0 = KERNEL_DS;
    tss.esp0 = PHYSICAL_START - terminal[term-1].term_pid * EIGHT_KB_BLOCK - MAGIC_SIZE;
    remap(VIRTUAL_START, PHYSICAL_START + terminal[term-1].term_pid * FOUR_MB_BLOCK);
    key_buffer = terminal[term-1].key_buffer;
    key_buffer_pos = terminal[term-1].key_buffer_pos;
    set_screen_pos(terminal[term-1].pos_x, terminal[term-1].pos_y);
    update_cursor_loc(get_screen_x(),get_screen_y());
    memcpy((uint8_t *)VIDEO, (uint8_t *)terminal[term-1].vid_mem, 2*NUM_ROWS*NUM_COLS);
    
    asm volatile ("movl %0, %%esp \n\t"
        "movl %1, %%ebp \n\t"
        :
        : "r"(esp_temp), "r"(ebp_temp)
        : "%eax"
      );

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

    clear_buffer();

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
