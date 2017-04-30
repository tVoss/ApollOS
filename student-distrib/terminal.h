#ifndef TERMINAL_H_
#define TERMINAL_H_

#include "types.h"
#include "keyboard.h"
#include "syscalls.h"
#include "x86_desc.h"

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
#define ONETWENTYEIGHT_MB 0x08000000
//#define WELCOME_SIZE	444

typedef struct {

	uint8_t num;	// which of the three terminals
	uint8_t key_buffer[KEY_BUFFER_SIZE];
	uint32_t key_buffer_pos;

	//cursor x,y location
	int pos_x;
	int pos_y;

	//stack frame values
	int32_t esp;
	int32_t ebp;

	//pcb value of first shell of each terminal
	pcb_t * top_pcb;

	// pointer to video memory for terminal
	uint8_t *vid_mem;

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
extern int32_t terminal_save(int term);
extern int32_t terminal_load(int term);
extern int32_t terminal_read (int32_t fd, void *buf, int32_t nbytes);
extern int32_t terminal_write (int32_t fd, const void *buf, int32_t nbytes);

#endif
