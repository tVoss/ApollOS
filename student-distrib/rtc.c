#include "rtc.h"

#include "interrupt_handlers.h"
#include "i8259.h"
#include "lib.h"
#include "x86_desc.h"

void init_rtc() {
    cli();

    // Turn RTC on
    outb(RTC_REG_B, RTC_PORT);
    char prev = inb(CMOS_PORT);
    outb(RTC_REG_B, RTC_PORT);
    outb(prev | 0x40, CMOS_PORT);

    // 2Hz initially
    rtc_set_frequency(2);

    // Enable interrupts
    enable_irq(RTC_IRQ_LINE);
    sti();
}

void rtc_set_frequency(unsigned int freq) {
    // Approximatly turn requested frequency to proper rate
    int rate = freq ? 16 : 0;
    while (freq >>= 1) {
        rate--;
    }
    rate &= 0xF;    // Keep it within bounds

    cli();
    outb(RTC_REG_A, RTC_PORT);
    char prev = inb(CMOS_PORT);
    outb(RTC_REG_A, RTC_PORT);
    outb((prev & 0xF0) | rate, CMOS_PORT);
    sti();
}

void rtc_handler(regs_t regs) {
    cli();

    // Checkpoint 1
    test_interrupts();

    // Throw away contents
    outb(RTC_REG_C, RTC_PORT);
    inb(CMOS_PORT);

    // Send eoi to PIC
    send_eoi(RTC_IRQ_LINE);

    sti();
}
