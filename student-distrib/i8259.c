/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask = 0xFF; 	/* IRQs 0-7 */
uint8_t slave_mask = 0xFF; 		/* IRQs 8-15 */

/*
 * i8259_init(void)
 *
 * DESCRIPTION: Initialize the 8259 PIC
 * 				Initialize the PIC by sending ICWs. Need to remap the
 *				PIC's IRQ numbers to our own.  Ensures proper IRQ is
 *				generated when a hardware interrupt happens.
 *
 * INPUTS: 	none
 * OUTPUTS: none
 *
 * SIDE EFFECTS: initalizes the PIC
 *
 */
void
i8259_init(void)
{
	// enable slave irq line (on line 2)
	enable_irq(SLAVE_IRQ_LINE);

	/* Initialization Control Word (ICW) */

	// ICW 1 - primary control word to intialize the PIC.
	// 		   put in the primary PIC command register.
	outb(ICW1,MASTER_8259_PORT);
	outb(ICW1,SLAVE_8259_PORT);

	// ICW 2 - used to amp the base address of the IVT that
	//		   the PIC are going to use.
	outb(ICW2_MASTER,MASTER_8259_PORT+1);
	outb(ICW2_SLAVE,SLAVE_8259_PORT+1);

	// ICW 3 - tells PICs what IRQ lines to use when communicating
	//		   with each other.
	outb(ICW3_MASTER,MASTER_8259_PORT+1);
	outb(ICW3_SLAVE,SLAVE_8259_PORT+1);

	// ICW 4 - controls how everything is to operate
	outb(ICW4,MASTER_8259_PORT+1);
	outb(ICW4,SLAVE_8259_PORT+1);
}

/*
 * enable_irq(uint32_t irq_num)
 *
 * DESCRIPTION: enables the specified irq by masking. sends
 *				updated mask (active low) to data port.
 *
 * INPUT: irq_num - what IRQ line to enable
 * OUTPUT: none
 *
 * SIDE EFFECTS: enables irq line
 *
 */
void
enable_irq(uint32_t irq_num)
{
	// check if irq_num is valid (0-7 for master, 8-15 for slave)
	if (irq_num >= 0 && irq_num <= 15){
		uint8_t mask = 0xFE;	// initial mask, only first line enabled
		int i;

		// master irq
		if (irq_num >= 0 && irq_num <= 7){
			for (i = 0; i < irq_num; i++){
				// left shift to enable next line,
				// add one to disable previous line
				mask = (mask << 1) + 1;
			}
			// send mask to PIC to enable irq line
			master_mask = master_mask & mask;
 			outb(master_mask, MASTER_8259_PORT + 1);
 			return;
		}

		// slave irq
		if (irq_num >= 8 && irq_num <= 15){
			irq_num = irq_num - 8;	// get 8-15 irq into 0-7 irq range
			for (i = 0; i < irq_num; i++){
				mask = (mask << 1) + 1;
			}
			// send mask to PIC to enable irq line
			slave_mask = slave_mask & mask;
 			outb(slave_mask, SLAVE_8259_PORT + 1);
 			return;
		}
	}
	return;
}

/*
 * disable_irq(uint32_t irq_num)
 *
 * DESCRIPTION: disables the specified irq by masking. sends
 *				updated mask (active low) to data port.
 *
 * INPUT: irq_num - what IRQ line to disable
 * OUTPUT: none
 *
 * SIDE EFFECTS: disables irq line
 *
 */
void
disable_irq(uint32_t irq_num)
{
	// check if irq is valid (0-7 for master, 8-15 for slave)
	if (irq_num >= 0 && irq_num <= 15){
		uint8_t mask = 0x01;	// initial mask, only first line disabled
		int i;

		// master irq
		if (irq_num >= 0 && irq_num <= 7){
			for (i = 0; i < irq_num; i++){
				// left shift to disable next line,
				// shfiting leaves previous bit enabled
				mask = (mask << 1);
			}
			// send mask to PIC to disable irq line
			master_mask = master_mask | mask;
 			outb(master_mask, MASTER_8259_PORT + 1);
 			return;
		}

		// slave irq
		if (irq_num >= 8 && irq_num <= 15){
			irq_num = irq_num - 8;	// get 8-15 irq into 0-7 irq range
			for (i = 0; i < irq_num; i++){
				mask = (mask << 1);
			}
			// send mask to PIC to enable irq line
			slave_mask = slave_mask | mask;
 			outb(slave_mask, SLAVE_8259_PORT + 1);
 			return;
		}
	}
	return;
}

/*
 * send_eoi(uint32_t irq_num)
 *
 * DESCRIPTION: send end-of-interrupt signal for the specified IRQ.
 *				The master has bounds from 0 to 7.  Slave has bounds
 *				from 8 to 15.
 *
 * INPUT: irq_num - what IRQ line to operate on
 * OUTPUT: none
 *
 * SIDE EFFECTS: sends EOI to PIC
 *
 */
void
send_eoi(uint32_t irq_num)
{
	// master bounds
	if ((irq_num >= 0) && (irq_num <= 7))
		outb( EOI | irq_num, MASTER_8259_PORT);
	// slave bounds
	if ((irq_num >= 8) && (irq_num <= 15)) {
		outb( EOI | (irq_num - 8), SLAVE_8259_PORT);
		outb( EOI + 2, MASTER_8259_PORT);
	}
}
