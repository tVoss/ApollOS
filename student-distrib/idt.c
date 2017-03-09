#include "idt.h"

#include "x86_desc.h"

CREATE_EXCEPTION(EX_DIVIDE_ERROR, "Divide Error Exception! (DE)");
CREATE_EXCEPTION(EX_DEBUG, "Debug Exception! (DB)");
CREATE_EXCEPTION(EX_NON_MASKABLE_INTERRUPT, "Non Maskable Interrupt! (NMI)");
CREATE_EXCEPTION(EX_BREAKPOINT, "Breakpoint Exception! (BP)");
CREATE_EXCEPTION(EX_OVERFLOW, "Overflow Excepion! (OF)");
CREATE_EXCEPTION(EX_BOUND_RANGE_EXCEEDED, "Bound Range Exceeded Exception! (BR)");
CREATE_EXCEPTION(EX_INVALID_OPCODE, "Invalid Opcode Exception! (UD)");
CREATE_EXCEPTION(EX_DEVICE_NOT_AVAILABLE, "Device Not Available Exception! (NM)");
CREATE_EXCEPTION(EX_DOUBLE_FAULT, "Double Fault Exception! (DF)");
CREATE_EXCEPTION(EX_COPROCESSOR_SEGMENT_OVERRUN, "Coprocessor Segment Overrun! (CS)");
CREATE_EXCEPTION(EX_INVALID_TSS, "Invalid TSS Exception! (TS)");
CREATE_EXCEPTION(EX_SEGMENT_NOT_PRESENT, "Segment Not Present! (NP)");
CREATE_EXCEPTION(EX_STACK_FAULT, "Stack Fault Exception! (SS)");
CREATE_EXCEPTION(EX_GENERAL_PROTECTION, "General Protection Exception! (GP)");
CREATE_EXCEPTION(EX_PAGE_FAULT, "Page Fault Exception! (PF)");
CREATE_EXCEPTION(EX_FLOATING_POINT_ERROR, "x87 Floating-Point Error! (MF)");
CREATE_EXCEPTION(EX_ALIGNMENT_CHECK, "Alignment Check Exception! (AC)");
CREATE_EXCEPTION(EX_MACHINE_CHECK, "Machine Check Exception! (MC)");
CREATE_EXCEPTION(EC_FLOATING_POINT, "SIMD Floating Point Exception! (XM)")

void init_idt() {

    int i;  // Loop variable

    lidt(idt_desc_ptr);

    for (i = 0; i < NUM_VEC; i++) {
        idt[i].seg_selector = KERNEL_CS;    // Set to kernet segment

        // Set reserved bits
        idt[i].reserved4 = 0x0;
        // 0 - 31 Trap ; 32 - 256 Interrupt
        idt[i].reserved3 = i < 32 || i == INT_SYSCALL
            ? 0x1   // 0 - 31 + 0x80 Trap
            : 0x0;  // 32 - 256 Interrupt
        idt[i].reserved2 = 0x1;
        idt[i].reserved1 = 0x1;
        idt[i].reserved0 = 0x0;
    }

}
