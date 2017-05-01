#include "syscalls.h"
#include "scheduler.h"
#include "terminal.h"
#include "multiboot.h"
#include "paging.h"
#include "x86_desc.h"
#include "rtc.h"
#include "rofs.h"
#include "i8259.h"

#define CMD_IN 0x34
#define CMD_REG 0x43
#define CHANNEL_0 0x40
#define CHANNEL_1 0x41
#define CHANNEL_2 0x42
#define CHANNEL_3 0x43
#define CHANNEL_0_IN 65535
#define MASK 0xFF

uint32_t current_terminal = 1;

/*
* void pit_init()
*   Inputs: NONE
*   Return Value: NONE
*	Function: Initializes and sets frequency for the programmable interval timer
*/
void pit_init(void)
{
	outb(CMD_IN,CMD_REG);	//square wave
	outb(CHANNEL_0_IN & MASK, CHANNEL_0);	//54ms
	outb(CHANNEL_0_IN >> 8, CHANNEL_0);
	enable_irq(PIT_IRQ_LINE);	//allows intrs
}

void pit_handler(void)
{
	cli();
	send_eoi(PIT_IRQ_LINE);
	uint8_t * screen;
	screen = VIDEO;
	uint32_t esp_temp;
	uint32_t ebp_temp;

	asm volatile ("movl %%esp,%0 \n\t"
				  "movl %%ebp,%1 \n\t"
				  : "=r"(esp_temp), "=r"(ebp_temp)
	); 

	//pcb = terminal[current_terminal].pcb;
	terminal[current_terminal].esp = esp_temp;
	terminal[current_terminal].ebp = ebp_temp;
	current_terminal++;

	if (current_terminal > 3)
	{
		current_terminal = 1;
	}

	while(terminal[current_terminal].num_processes == 0)
	{
		current_terminal = (current_terminal + 1)%3;
	}

	//pcb = terminal[current_terminal].pcb;
	tss.ss0 = KERNEL_DS;
	tss.esp0 = PHYSICAL_START - (EIGHT_KB_BLOCK * terminal[current_terminal].term_pid) - 4;
	remap(VIRTUAL_START, PHYSICAL_START + (terminal[current_terminal].term_pid * FOUR_MB_BLOCK));
	vidmap(&screen);

	if(terminal[current_terminal].num != current_terminal)
	{
		remapVideo((uint32_t)screen, (uint32_t)terminal[current_terminal].vid_mem);
	}
	else
	{
		remapVideo((uint32_t)screen, (uint32_t)VIDEO);
	}
	asm volatile ("movl %0, %%esp \n\t"
			  	  "movl %1, %%ebp \n\t"
			  	  :
			      : "r"(esp_temp), "r"(ebp_temp)
				  : "%eax", "%esp", "%ebp"
	); 
	sti();
}



