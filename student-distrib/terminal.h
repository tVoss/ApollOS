#ifndef TERMINAL_H_
#define TERMINAL_H_

#include "types.h"
#include "keyboard.h"

#define MAX_TERMINALS 	3
#define TERM_MEM 		0x6400000		// 100MB
#define TERM_1_MEM 		0x6401000		// 100MB+4KB
#define TERM_2_MEM 		0x6402000		// 100MB+2*4KB
#define TERM_3_MEM 		0x6403000		// 100MB+3*4KB
#define KEY_BUFFER_SIZE 128
#define VIDEO 			0xB8000
#define NUM_COLS 		80
#define NUM_ROWS 		25
#define ATTRIB_G		0xA
#define ATTRIB_Y 		0xE
#define ATTRIB_B 		0xB
#define WELCOME_SIZE	444

typedef struct {

	uint8_t num;	// which of the three terminals
	uint8_t key_buffer[KEY_BUFFER_SIZE];
	uint32_t key_buffer_pos;
	uint8_t enter_pressed;

	// cursor x,y location
	int pos_x;
	int pos_y;

	uint8_t *vid_mem;	// pointer to video memory for terminal

	int init;	// has the terminal been launched? yes(1), no(0)

} terminal_t;

extern volatile terminal_t terminal[MAX_TERMINALS];
extern volatile int term_cur;
/* key buffer */
extern volatile uint8_t *key_buffer;

/* terminal driver system calls */
extern int32_t terminal_open(const int8_t *filename);
extern int32_t terminal_close(int32_t fd);
extern void init_terminals();
extern int32_t switch_terminal(int term);
extern int32_t terminal_start(int term);
extern int32_t terminal_read (int32_t fd, void *buf, int32_t nbytes);
extern int32_t terminal_write (int32_t fd, const void *buf, int32_t nbytes);

#endif
