#include "idt.h"

#include "interrupt_handlers.h"
#include "lib.h"
#include "x86_desc.h"

// Exception names and values taken from IA-32 Reference Manual Section 6.15
CREATE_EXCEPTION(EX_DIVIDE_ERROR,                   (DE) Divide Error Exception!);
CREATE_EXCEPTION(EX_DEBUG,                          (DB) Debug Exception!);
CREATE_EXCEPTION(EX_NON_MASKABLE_INTERRUPT,         (NI) Non Maskable Interrupt!);
CREATE_EXCEPTION(EX_BREAKPOINT,                     (BP) Breakpoint Exception!);
CREATE_EXCEPTION(EX_OVERFLOW,                       (OF) Overflow Excepion!);
CREATE_EXCEPTION(EX_BOUND_RANGE_EXCEEDED,           (BR) Bound Range Exceeded Exception!);
CREATE_EXCEPTION(EX_INVALID_OPCODE,                 (UD) Invalid Opcode Exception!);
CREATE_EXCEPTION(EX_DEVICE_NOT_AVAILABLE,           (NM) Device Not Available Exception!);
CREATE_EXCEPTION(EX_DOUBLE_FAULT,                   (DF) Double Fault Exception!);
CREATE_EXCEPTION(EX_COPROCESSOR_SEGMENT_OVERRUN,    (CS) Coprocessor Segment Overrun!);
CREATE_EXCEPTION(EX_INVALID_TSS,                    (TS) Invalid TSS Exception!);
CREATE_EXCEPTION(EX_SEGMENT_NOT_PRESENT,            (NP) Segment Not Present!);
CREATE_EXCEPTION(EX_STACK_FAULT,                    (SS) Stack Fault Exception!);
CREATE_EXCEPTION(EX_GENERAL_PROTECTION,             (GP) General Protection Exception!);
CREATE_EXCEPTION(EX_PAGE_FAULT,                     (PF) Page Fault Exception!);
CREATE_EXCEPTION(EX_UNKNOWN,                        (UN) Unknown Exception!);
CREATE_EXCEPTION(EX_FLOATING_POINT_ERROR,           (MF) x87 Floating-Point Error!);
CREATE_EXCEPTION(EX_ALIGNMENT_CHECK,                (AC) Alignment Check Exception!);
CREATE_EXCEPTION(EX_MACHINE_CHECK,                  (MC) Machine Check Exception!);
CREATE_EXCEPTION(EX_FLOATING_POINT,                 (XM) SIMD Floating Point Exception!);

void EX_GENERIC() {
    cli();
    printf("An unknown interrupt occured!\n");
    sti();
}

void init_idt() {

    int i;

    // Set up all of the exceptions
    for (i = 0; i < NUM_VEC; i++) {
        // Gate reference in IA-32 Manual Section 6.11

        idt[i].seg_selector = KERNEL_CS;    // Set to kernet segment

        // Set reserved bits
        idt[i].reserved4 = 0x0;
        // 0 - 31 Trap ; 32 - 256 Interrupt
        idt[i].reserved3 = i < 32 || i == INT_SYSCALL
            ? 0x1   // 0 - 31 + 0x80 Trap
            : 0x0;  // 32 - 256 Interrupt
        idt[i].reserved2 = 0x1;
        idt[i].reserved1 = 0x1;

        // 32 bit gate
        idt[i].size = 0x1;

        // Last reserved bit
        idt[i].reserved0 = 0x0;

        // Only syscalls come from user space
        idt[i].dpl = i == INT_SYSCALL ? 0x3 : 0x0;

        // This interrupt is present
        idt[i].present = 0x1;

        if (i >= 32) {
            SET_IDT_ENTRY(idt[i], EX_GENERIC);
        }
    }

    // Exception numbers taken from 6.15
    SET_IDT_ENTRY(idt[0x00], EX_DIVIDE_ERROR);
    SET_IDT_ENTRY(idt[0x01], EX_DEBUG);
    SET_IDT_ENTRY(idt[0x02], EX_NON_MASKABLE_INTERRUPT);
    SET_IDT_ENTRY(idt[0x03], EX_BREAKPOINT);
    SET_IDT_ENTRY(idt[0x04], EX_OVERFLOW);
    SET_IDT_ENTRY(idt[0x05], EX_BOUND_RANGE_EXCEEDED);
    SET_IDT_ENTRY(idt[0x06], EX_INVALID_OPCODE);
    SET_IDT_ENTRY(idt[0x07], EX_DEVICE_NOT_AVAILABLE);
    SET_IDT_ENTRY(idt[0x08], EX_DOUBLE_FAULT);
    SET_IDT_ENTRY(idt[0x09], EX_COPROCESSOR_SEGMENT_OVERRUN);
    SET_IDT_ENTRY(idt[0x0A], EX_INVALID_TSS);
    SET_IDT_ENTRY(idt[0x0B], EX_SEGMENT_NOT_PRESENT);
    SET_IDT_ENTRY(idt[0x0C], EX_STACK_FAULT);
    SET_IDT_ENTRY(idt[0x0D], EX_GENERAL_PROTECTION);
    SET_IDT_ENTRY(idt[0x0E], EX_PAGE_FAULT);
    SET_IDT_ENTRY(idt[0x0F], EX_UNKNOWN);
    SET_IDT_ENTRY(idt[0x10], EX_FLOATING_POINT_ERROR);
    SET_IDT_ENTRY(idt[0x11], EX_ALIGNMENT_CHECK);
    SET_IDT_ENTRY(idt[0x12], EX_MACHINE_CHECK);
    SET_IDT_ENTRY(idt[0x13], EX_FLOATING_POINT);

    // Special entries
    SET_IDT_ENTRY(idt[INT_RTC], handle_rtc);

    lidt(idt_desc_ptr);

}
