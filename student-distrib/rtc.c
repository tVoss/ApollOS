#include "rtc.h"

#include "interrupt_handlers.h"
#include "lib.h"
#include "x86_desc.h"

void init_rtc() {
    SET_IDT_ENTRY(idt[INT_RTC], handle_rtc);
}

void rtc_handler(regs_t regs) {
    test_interrupts();
}
