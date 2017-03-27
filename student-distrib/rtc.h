#ifndef RTC_H_
#define RTC_H_

#include "idt.h"

#define RTC_PORT    0x70
#define CMOS_PORT   0x71
#define RTC_REG_A   0x8A
#define RTC_REG_B   0x8B
#define RTC_REG_C   0x8C
#define RTC_REG_D   0x8D

#define RTC_ENABLE_INTERRUPT    0x40
#define RTC_SET_RATE            0xF0

/* All possible frequencies */
typedef enum rtc_freq {
    F2HZ        = 15,
    F4HZ        = 14,
    F8HZ        = 13,
    F16HZ       = 12,
    F32HZ       = 11,
    F64HZ       = 10,
    F128HZ      = 9,
    F256HZ      = 8,
    F512HZ      = 7,
    F1024HZ     = 6,
    F2048HZ     = 5,
    F4096HZ     = 4,
    F8192HZ     = 3,
    F0HZ        = 0
} rtc_freq_t;

/* Initialize the Real Time Clock */
extern void init_rtc();
/* Set the frequency of the clock */
extern void rtc_set_frequency(rtc_freq_t freq);
/* Handle clock interrupts */
extern void rtc_handler();
/* Opens file */
extern int32_t rtc_open(void);
/* Closes file*/
extern int32_t rtc_close(void);
/* Reads rtc after interrupt*/
extern int32_t rtc_read(int32_t* buf, int32_t nbytes);
/* Writes frequency of rtc*/
extern int32_t rtc_write(int32_t* buf, int32_t nbytes);

#endif
