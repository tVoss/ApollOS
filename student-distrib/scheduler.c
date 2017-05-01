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

    // Get the next terminal to inspect
	next_terminal = (current_terminal + 1) % 3;

    // Find an active terminal
	while(terminal[next_terminal].init != 1)
	{
		next_terminal = (next_terminal + 1) % 3;
	}

    // We're not changing anything
    if (next_terminal == current_terminal) {
        sti();
        return;
    }

	remap(VIRTUAL_START, EIGHT_MB_BLOCK + terminal[next_terminal].term_pid * FOUR_MB_BLOCK);

	if(terminal[next_terminal].num != term_cur) {
        // Video should point to terminal memory
		remapWithPageTable((uint32_t) VIDEO, (uint32_t) terminal[next_terminal].vid_mem);
	} else {
        // Video should point to actual video memory
        remapWithPageTable(VIDEO, VIDEO);
    }

	tss.ss0 = KERNEL_DS;
	//tss.esp0 = EIGHT_MB_BLOCK + EIGHT_KB_BLOCK * terminal[next_terminal].term_pid -4;
	//tss.esp0 = PHYSICAL_START - EIGHT_KB_BLOCK * terminal[next_terminal].term_pid - 4;
    tss.esp0 = PHYSICAL_START - terminal[next_terminal].term_pid * EIGHT_KB_BLOCK - MAGIC_SIZE;
    // Store current term esp and ebp
    asm volatile(
        "movl %%esp, %0;"
        "movl %%ebp, %1;"
        :"=r"(terminal[current_terminal].esp), "=r"(terminal[current_terminal].ebp)
        :
    );

    // Update terminal being processes
    current_terminal = next_terminal;

    // Update esp and ebp
    asm volatile(
        "movl %0, %%esp;"
        "movl %1, %%ebp;"
        :
        :"r"(terminal[current_terminal].esp), "r"(terminal[current_terminal].ebp)
    );

    sti();
	return;
}
