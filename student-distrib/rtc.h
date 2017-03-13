#ifndef RTC_H_
#define RTC_H_

#include "idt.h"

#define RTC_PORT    0x70
#define CMOS_PORT   0x71
#define RTC_REG_A   0x8A
#define RTC_REG_B   0x8B
#define RTC_REG_C   0x8C
#define RTC_REG_D   0x8D

extern void init_rtc();
extern void rtc_set_frequency(unsigned int freq);
extern void rtc_handler(regs_t regs);

#endif
