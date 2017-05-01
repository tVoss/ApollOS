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
#define IN_USE 1
#define UNUSED 0

uint32_t current_terminal = 0;
uint32_t next_terminal = 0;

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
	next_terminal = current_terminal;
	if(next_terminal >= 2)
	{
		next_terminal = 0;
	}
	else
	{
		next_terminal++;
	}
	while(terminal[next_terminal].init != 1)
	{
		next_terminal = (next_terminal + 1) % 3;
	}
	remap(VIRTUAL_END, EIGHT_MB_BLOCK + terminal[current_terminal].term_pid * FOUR_MB_BLOCK);
	//uint8_t * screen_start;
	//vidmap(&screen_start);
	//pcb_t * current_pcb = get_pcb(terminal[current_terminal].term_pid);
	//current_terminal = next_terminal;
	//pcb_t * next_pcb = get_pcb(terminal[next_terminal].term_pid);

	if(terminal[next_terminal].num != term_cur)
	{
		remapWithPageTable((uint32_t) VIRTUAL_END, (uint32_t) terminal[next_terminal].vid_mem);
	}

	tss.ss0 = KERNEL_DS;
	//tss.esp0 = EIGHT_MB_BLOCK + EIGHT_KB_BLOCK * terminal[next_terminal].term_pid -4;
	tss.esp0 = PHYSICAL_START - EIGHT_KB_BLOCK * terminal[next_terminal].term_pid - 4;
	asm volatile(
						 "movl %%esp, %0;"
						 "movl %%ebp, %1;"
						 :"=r"(terminal[current_terminal].esp), "=r"(terminal[current_terminal].ebp)
						 :
						 );

  asm volatile(
						 "movl %0, %%esp;"
						 "movl %1, %%ebp;"
						 :
						 :"r"(terminal[next_terminal].esp), "r"(terminal[next_terminal].ebp)
					 );

	current_terminal = next_terminal;
	return;
}
/*
static int32_t schedule_array[MAX_PROCESSES] = {[0 ... MAX_PROCESSES -1] = UNUSED};
static uint32_t current_process = 0;

uint32_t scheduler_c(void)
{
		do
		{
			current_process = (current_process + 1) % MAX_PROCESSES;
		} while (schedule_array[current_process] == UNUSED);

		return current_process;
}

void schedule_new(uint32_t pid)
{
	schedule_array[pid] = IN_USE;
}

void unschedule_old(uint32_t pid)
{
	schedule_array[pid] = UNUSED;
}
*/
