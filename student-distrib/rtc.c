#include "rtc.h"

#include "interrupt_handlers.h"
#include "i8259.h"
#include "lib.h"
#include "x86_desc.h"

/*
 * init_rtc(void)
 *
 * DESCRIPTION: Initialize the Real Time Clock, enables
 *              its PIC line and sets the frequency to 2Hz
 *
 * INPUTS: 	none
 * OUTPUTS: none
 *
 * SIDE EFFECTS: Initalizes the RTC
 *
 */
void init_rtc() {
    cli();

    // Turn RTC on
    outb(RTC_REG_B, RTC_PORT);
    char prev = inb(CMOS_PORT);
    outb(RTC_REG_B, RTC_PORT);
    outb(prev | RTC_ENABLE_INTERRUPT, CMOS_PORT);

    // 2Hz initially
    rtc_set_frequency(F2HZ);

    // Enable interrupts
    enable_irq(RTC_IRQ_LINE);
    sti();
}

/*
 * i8259_init(freq)
 *
 * DESCRIPTION: Changes te frequency of the RTC to one
 *              of 14 valid options (0Hz - 8192Hz)
 *
 * INPUTS: 	freq - The value of the desired frequency
 * OUTPUTS: none
 *
 * SIDE EFFECTS: Sets the frequency of the RTC
 *
 */
void rtc_set_frequency(rtc_freq_t freq) {
    cli();
    outb(RTC_REG_A, RTC_PORT);
    char prev = inb(CMOS_PORT);
    outb(RTC_REG_A, RTC_PORT);
    outb((prev & RTC_SET_RATE) | freq, CMOS_PORT);
    sti();
}

/*
 * rtc_handler(void)
 *
 * DESCRIPTION: Processes interrupts generated by the RTC
 *
 * INPUTS: 	none
 * OUTPUTS: none
 *
 * SIDE EFFECTS: N/A
 *
 */
void rtc_handler() {
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
